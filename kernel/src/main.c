/*
	VitaShell
	Copyright (C) 2015-2016, TheFloW

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <vitasdkkern.h>

#include <stdio.h>
#include <string.h>

#include <taihen.h>

#include "appmgr_kernel.h"

#define printf ksceDebugPrintf

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

void *(* sceAppMgrFindProcessInfoByPid)(void *data, SceUID pid);
int (* sceAppMgrMountById)(SceUID pid, void *info, int id, const char *titleid, const char *path, const char *desired_mount_point, const void *klicensee, char *mount_point);
int (* _ksceKernelGetModuleInfo)(SceUID pid, SceUID modid, SceKernelModuleInfo *info);

int ksceRtcSetCurrentSecureTick(unsigned int* timestamp);
int ksceRtcGetCurrentSecureTick(unsigned int* timestamp);

static int hook = -1;
static tai_hook_ref_t ref_hook;

tai_module_info_t tai_info;

int fakeTime[2];
int spoofTime = 0;

int getTimePatched(unsigned int* timestamp)
{
	
	int ret;
	ret = TAI_CONTINUE(int, ref_hook,timestamp);
	if(spoofTime)
	{
		printf("[TROPHAXSE] Faking timestamp to %x%x and Return 0",fakeTime[1],fakeTime[0]);
		timestamp[0] = fakeTime[0];
		timestamp[1] = fakeTime[1];
		return 0;
	}
	return ret;
}

int kFakeTimes(int value)
{
	spoofTime = value;
	return 0;
}

int kSetTrophyTimes(unsigned int timestamp1, unsigned int timestamp2)
{
	
    
    fakeTime[0] = timestamp1;
    fakeTime[1] = timestamp2;
    
    return 0;
}



int _sceAppMgrKernelMountById(SceAppMgrMountIdArgs *args) {
  int res;

  res = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0xD269F915, (uintptr_t *)&_ksceKernelGetModuleInfo);
  if (res < 0)
    res = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xDAA90093, (uintptr_t *)&_ksceKernelGetModuleInfo);
  if (res < 0)
    return res;

  // Module info
  SceKernelModuleInfo mod_info;
  mod_info.size = sizeof(SceKernelModuleInfo);
  res = _ksceKernelGetModuleInfo(KERNEL_PID, tai_info.modid, &mod_info);
  if (res < 0)
    return res;

  uint32_t appmgr_data_addr = (uint32_t)mod_info.segments[1].vaddr;
  
  SceUID process_id = ksceKernelGetProcessId();

  void *info = sceAppMgrFindProcessInfoByPid((void *)(appmgr_data_addr + 0x500), process_id);
  if (!info)
    return -1;

  char process_titleid[12];
  char path[256];
  char desired_mount_point[16];
  char mount_point[16];
  char klicensee[16];

  memset(mount_point, 0, sizeof(mount_point));

  if (args->process_titleid)
    ksceKernelStrncpyUserToKernel(process_titleid, (uintptr_t)args->process_titleid, 11);
  if (args->path)
    ksceKernelStrncpyUserToKernel(path, (uintptr_t)args->path, 255);
  if (args->desired_mount_point)
    ksceKernelStrncpyUserToKernel(desired_mount_point, (uintptr_t)args->desired_mount_point, 15);
  if (args->klicensee)
    ksceKernelMemcpyUserToKernel(klicensee, (uintptr_t)args->klicensee, 0x10);
  
  res = sceAppMgrMountById(process_id, info + 0x580, args->id, args->process_titleid ? process_titleid : NULL, args->path ? path : NULL,
                           args->desired_mount_point ? desired_mount_point : NULL, args->klicensee ? klicensee : NULL, mount_point);

  if (args->mount_point)
    ksceKernelStrncpyKernelToUser((uintptr_t)args->mount_point, mount_point, 15);

  return res;
}

int sceAppMgrKernelMountById(SceAppMgrMountIdArgs *args) {
  uint32_t state;
  ENTER_SYSCALL(state);

  SceAppMgrMountIdArgs k_args;
  ksceKernelMemcpyUserToKernel(&k_args, (uintptr_t)args, sizeof(SceAppMgrMountIdArgs));

  int res = ksceKernelRunWithStack(0x2000, (void *)_sceAppMgrKernelMountById, &k_args);

  EXIT_SYSCALL(state);
  return res;
}


void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
	printf("TrophaxSE Kernel Module loaded!\n");
	hook = taiHookFunctionExportForKernel(KERNEL_PID,
		&ref_hook, 
		"SceRtc",
		0x0351D827, // SceRtcForDriver
		0x401C0954, // ksceRtcGetCurrentSecureTick
		getTimePatched);
	printf("[TROPHAXSE] hook: %x\n",hook);

  // Get tai module info

  tai_info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceAppMgr", &tai_info) < 0)
    return SCE_KERNEL_START_SUCCESS;

  switch (tai_info.module_nid) {
    case 0xDBB29DB7: // 3.60 retail
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE1, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19B51, (uintptr_t *)&sceAppMgrMountById);
      break;
      
    case 0x1C9879D6: // 3.65 retail
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE1, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19E61, (uintptr_t *)&sceAppMgrMountById);
      break;
      
    case 0x54E2E984: // 3.67 retail
    case 0xC3C538DE: // 3.68 retail
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE1, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19E6D, (uintptr_t *)&sceAppMgrMountById);
      break;
	  
    case 0x321E4852: // 3.69 retail
	case 0x700DA0CD: // 3.70 retail
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x2DE9, (uintptr_t *)&sceAppMgrFindProcessInfoByPid);
      module_get_offset(KERNEL_PID, tai_info.modid, 0, 0x19E95, (uintptr_t *)&sceAppMgrMountById);
      break;
  }
  



  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (hook >= 0) taiHookReleaseForKernel(hook, ref_hook);   
  return SCE_KERNEL_STOP_SUCCESS;
}
		