
#ifndef __SCEAPPMGR_KERNEL_H__
#define __SCEAPPMGR_KERNEL_H__

typedef struct {
  int id;
  const char *process_titleid;
  const char *path;
  const char *desired_mount_point;
  const void *klicensee;
  char *mount_point;
} SceAppMgrMountIdArgs;


int sceAppMgrKernelMountById(SceAppMgrMountIdArgs *args);

#endif