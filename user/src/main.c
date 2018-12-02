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


#include <psp2/kernel/modulemgr.h>
#include <vitasdk.h>
#include "appmgr_user.h"


int SetTrophyTimes(unsigned int timestamp1,unsigned int timestamp2)
{
	return kSetTrophyTimes(timestamp1,timestamp2);
}

int sceAppMgrUserMountById(SceAppMgrMountIdArgs *args) {
  return sceAppMgrKernelMountById(args);
}


void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  return SCE_KERNEL_STOP_SUCCESS;
}
