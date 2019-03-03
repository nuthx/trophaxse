#include "trophy.h"

#include "listbox.h"

#include "np.h"
#include "rtc.h"

#include <appmgr_kernel.h>
#include <appmgr_user.h>

#include <vita2d.h>

#include <taihen.h>

#include <psp2/appmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/rtc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int kernel_modid = -1, user_modid = -1;
static char current_mount[16] = {};
static SceNpTrophyContext trophy_context = -1;

extern int infolog(const char *str, ...);

void *read_file(const char *file, int *size) {
    int file_size;
    void *data;
    SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0777);
    if (fd < 0)
        return NULL;
    file_size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);
    data = malloc(file_size);
    sceIoRead(fd, data, file_size);
    sceIoClose(fd);
    *size = file_size;
    return data;
}

int write_file(char *file, void *buf, int size) {
    int written;
    SceUID fd = sceIoOpen(file, SCE_O_RDWR | SCE_O_CREAT, 0777);
    if (fd < 0)
        return fd;
    written = sceIoWrite(fd, buf, size);
    sceIoClose(fd);
    return written;
}

int get_file_size(const char *file) {
    int file_size;
    SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0777);
    if (fd < 0)
        return -1;
    file_size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoClose(fd);
    return file_size;
}

int pfs_mount(const char *path, const char *titleid) {
    const int knomn_pfs_ids[] = {
        0x12F,
        0x3E8,
    };
    int ret;
    char klicensee[0x10] = {};
    SceAppMgrMountIdArgs args;

    args.process_titleid = titleid;
    args.path = path;
    args.desired_mount_point = NULL;
    args.klicensee = klicensee;
    args.mount_point = current_mount;

    for (int i = 0; i < sizeof(knomn_pfs_ids) / sizeof(int); i++) {

        args.id = knomn_pfs_ids[i];

        ret = sceAppMgrUserMountById(&args);

        if (ret >= 0) {
            return ret;
        }
    }

    return ret;
}

int trophy_games_list(trophy_gameinfo_t **games) {
    int count = 0, cap = 0;
    int res;
    int dfd = sceIoDopen("ur0:/user/00/trophy/data/");
    do {
        SceIoDirent dir = {};
        res = sceIoDread(dfd, &dir);
        if (res > 0 && SCE_S_ISDIR(dir.d_stat.st_mode) && strlen(dir.d_name) == 12 && dir.d_name[9] == '_') {
            char path[256];
            char titledest[256];
            int size;
            snprintf(path, 256, "ur0:/user/00/trophy/conf/%s/TROP.SFM", dir.d_name);
            char *sfm = read_file(path, &size);
            int len = strstr(sfm, "</title-name>") - (strstr(sfm, "<title-name>") + sizeof("<title-name>")) - 1;
            len += 2;
            memcpy(&titledest, strstr(sfm, "<title-name>") + sizeof("<title-name>") - 1, len);
            titledest[len] = 0;
            free(sfm);

            if (count >= cap) {
                cap += 8;
                *games = (trophy_gameinfo_t*)realloc(*games, sizeof(trophy_gameinfo_t) * cap);
            }
            snprintf((*games)[count].name, 256, "%s", titledest);
            snprintf((*games)[count++].trophyid, 13, "%s", dir.d_name);
        }
    } while (res > 0);
    sceIoDclose(dfd);
    return count;
}

int trophy_prepare(const char *trophyid, char game_titleid[16]) {
    int ret;
    int unmounted = 0;
    SceUChar8 commid[16] = {};
    SceUChar8 commsign[160] = {};
    char titleid[16] = {};
    char path[256] = {};
    int prepared_before = 0;

    infolog("Searching game for trophy id %s", trophyid);

    ret = sceAppMgrUmount("app0:");
    if (ret < 0) goto Fail;
    unmounted = 1;

    sceAppMgrAppParamGetString(0, 12, titleid, 16);

    if (kernel_modid < 0) {
        char kplugin_path[256];
        sprintf(kplugin_path, "ux0:app/%s/module/kernel.skprx", titleid);
        kernel_modid = taiLoadStartKernelModule(kplugin_path, 0, NULL, 0);
    }

    if (user_modid < 0) {
        char uplugin_path[256];
        sprintf(uplugin_path, "ux0:app/%s/module/user.suprx", titleid);
        user_modid = sceKernelLoadStartModule(uplugin_path, 0, NULL, 0, NULL, NULL);
    }

    {
        SceUID fd;
        char trptranspath[256] = {};
        char trptitlepath[256] = {};
        char trophy_path[256] = {};

        memcpy(&commid, trophyid, strlen(trophyid) + 1);
        snprintf(trophy_path, 256, "ur0:user/00/trophy/data/%s/", commid);

        ret = pfs_mount(trophy_path, titleid);
        if (ret < 0)
            goto Fail;

        snprintf(trptitlepath, 256, "%s/TRPTITLE.DAT", current_mount);

        fd = sceIoOpen(trptitlepath, SCE_O_RDONLY, 0777);
        sceIoLseek(fd, 0x290, SCE_SEEK_SET);
        sceIoRead(fd, game_titleid, 0x0A);
        if (memcmp(game_titleid, "TROPHAXSE", 9) == 0) {
            prepared_before = 1;
            sceIoLseek(fd, 0x2D0, SCE_SEEK_SET);
            sceIoRead(fd, game_titleid, 0x0A);
        }
        sceIoClose(fd);

        sprintf(trptranspath, "%s/TRPTRANS.DAT", current_mount);

        fd = sceIoOpen(trptranspath, SCE_O_RDONLY, 0777);
        sceIoLseek(fd, 0x190, SCE_SEEK_SET);
        sceIoRead(fd, commsign, 160);
        sceIoClose(fd);
        sceAppMgrUmount(current_mount);
    }

    do {
        char location[256] = {};
        char check_path[256] = {};
        int size;

        snprintf(check_path, 256, "ux0:/app/%s/sce_sys/trophy/%s/TROPHY.TRP", game_titleid, commid);
        if (get_file_size(check_path) >= 0) {
            sprintf(location, "ux0:/app");
            goto Found;
        }

        snprintf(check_path, 256, "gro0:/app/%s/sce_sys/trophy/%s/TROPHY.TRP", game_titleid, commid);
        if (get_file_size(check_path) >= 0) {
            sprintf(location, "gro0:/app");
            goto Found;
        }

        snprintf(check_path, 256, "ur0:/app/%s/sce_sys/trophy/%s/TROPHY.TRP", game_titleid, commid);
        if (get_file_size(check_path) >= 0) {
            sprintf(location, "ur0:/app");
            goto Found;
        }

        snprintf(check_path, 256, "pd0:/app/%s/sce_sys/trophy/%s/TROPHY.TRP", game_titleid, commid);
        if (get_file_size(check_path) >= 0) {
            sprintf(location, "pd0:/app"); //Welcome Park
            goto Found;
        }
        infolog("Cound not find %s (Possibly game not installed?)\n", game_titleid);
        if (prepared_before) {
            sceIoMkdir(path, 0006);
            snprintf(path, 256, "ux0:app/%s/sce_sys/trophy/%s/TROPHY.TRP", titleid, commid);
            size = get_file_size(path);
            if (size > 0) {
                infolog("But %s(%s) is already prepared for unlocking\n", commid, game_titleid);
                break;
            }
        } else goto Fail;

    Found:
        infolog("Found - %s is in %s/%s\n", commid, location, game_titleid);

        snprintf(path, 256, "%s/%s", location, game_titleid);
        ret = sceAppMgrGameDataMount(path, 0, 0, current_mount); //GameDataMount mounts WITH patches, so no need to check for them
        if (ret < 0) goto Fail;

        snprintf(path, 256, "%s/sce_sys/trophy/%s/TROPHY.TRP", current_mount, commid);
        size = get_file_size(path);
        if (size < 0) goto Fail;
        {
            char *trpfile = (char *)read_file(path, &size);
            if (trpfile == NULL) goto Fail;

            snprintf(path, 256, "ux0:app/%s/sce_sys", titleid);
            sceIoMkdir(path, 0006);
            snprintf(path, 256, "ux0:app/%s/sce_sys/trophy", titleid);
            sceIoMkdir(path, 0006);
            snprintf(path, 256, "ux0:app/%s/sce_sys/trophy/%s", titleid, commid);
            sceIoMkdir(path, 0006);
            snprintf(path, 256, "ux0:app/%s/sce_sys/trophy/%s/TROPHY.TRP", titleid, commid);
            ret = write_file(path, trpfile, size);
            free(trpfile);
            if (ret < 0) goto Fail;
        }
        ret = sceAppMgrUmount(current_mount);
        if (ret < 0) goto Fail;
    } while(0);

    // remount app0
    // CelesteBlue TAKE NOTES.
    snprintf(path, 256, "ux0:/app/%s", titleid);
    ret = pfs_mount(path, titleid);
    if (ret < 0) goto Fail;
    unmounted = 0;

    {
        infolog("Initializing NP Trophy...");
        SceNpCommunicationId np_commid = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0, 0, 0};
        SceNpCommunicationSignature np_commsign;
        SceNpTrophySetupDialogParam setupParam;
        SceNpTrophySetupDialogResult setupResult = {};

        ret = sceNpTrophyInit(NULL);
        if (ret < 0) goto Fail;

        memcpy(np_commid.data, commid, 9);
        memcpy(np_commsign.data, commsign, 160);

        ret = sceNpTrophyCreateContext(&trophy_context, &np_commid, &np_commsign, 0);
        if (ret < 0) goto Fail;

        sceNpTrophySetupDialogParamInit(&setupParam);

        setupParam.context = trophy_context;

        ret = sceCommonDialogSetConfigParam(&(SceCommonDialogConfigParam){});
        if (ret < 0) {
            sceNpTrophySetupDialogTerm();
            goto Fail;
        }

        ret = sceNpTrophySetupDialogInit(&setupParam);
        if (ret < 0) {
            sceNpTrophySetupDialogTerm();
            goto Fail;
        }

        while (1) {
            SceCommonDialogStatus cd_status;
            cd_status = sceNpTrophySetupDialogGetStatus();
            if (cd_status == SCE_COMMON_DIALOG_STATUS_FINISHED)
                break;
            vita2d_common_dialog_update();
        }

        ret = sceNpTrophySetupDialogGetResult(&setupResult);
        if (ret < 0) {
            sceNpTrophySetupDialogTerm();
            goto Fail;
        }

        ret = sceNpTrophySetupDialogTerm();
        if (ret < 0) goto Fail;

        if (setupResult.result != SCE_COMMON_DIALOG_RESULT_OK) goto Fail;
    }
    infolog("Finished.");
    return 0;

Fail:
    if (unmounted) {
        snprintf(path, 256, "ux0:/app/%s", titleid);
        pfs_mount(path, titleid);
    }
    trophy_finish();
    return -1;
}

void trophy_finish() {
    if (trophy_context >= 0) {
        sceNpTrophyDestroyContext(trophy_context);
        trophy_context = -1;
    }
    sceNpTrophyTerm();
    if (user_modid >= 0) {
        sceKernelStopUnloadModule(user_modid, 0, NULL, 0, NULL, NULL);
        user_modid = -1;
    }
    if (kernel_modid >= 0) {
        taiStopUnloadKernelModule(kernel_modid, 0, NULL, 0, NULL, NULL);
        kernel_modid = -1;
    }
}

static void copy_data(trophy_detail_t *detail, SceNpTrophyDetails *trophy_detail, SceNpTrophyData *trophy_data) {
    const char *grade_str[] = {"Unkw", "Plat", "Gold", "Silv", "Bron"};
    detail->id = trophy_detail->trophyId;
    detail->grade = trophy_detail->trophyGrade;
    detail->unlocked = trophy_data->unlocked;
    if (trophy_detail->hidden && trophy_detail->name[0] == 0)
        snprintf(detail->name, 128, "%lu. Hidden Trophy", trophy_detail->trophyId);
    else
        snprintf(detail->name, 128, "%lu. %s[%s] %s", trophy_detail->trophyId, trophy_data->unlocked ? "(Unlocked)" : "", grade_str[trophy_detail->trophyGrade], trophy_detail->name);
    snprintf(detail->description, 128, "%s", trophy_detail->description);
}

int trophy_list(trophy_detail_t **details, int only_unlockable) {
    SceUInt32 i;
    int count = 0;
    SceNpTrophyGameDetails game_detail = {0};
    SceNpTrophyDetails trophy_detail = {0};
    SceNpTrophyData trophy_data = {0};
    SceNpTrophyHandle handle;

    sceNpTrophyCreateHandle(&handle);
    game_detail.size = sizeof(SceNpTrophyGameDetails);
    sceNpTrophyGetGameInfo(trophy_context, handle, &game_detail, NULL);

    *details = (trophy_detail_t*)calloc(game_detail.numTrophies, sizeof(trophy_detail_t));
    trophy_detail.size = sizeof(SceNpTrophyDetails);
    trophy_data.size = sizeof(SceNpTrophyData);
    infolog("Obtaining trophy names...");
    for (i = 0; i < game_detail.numTrophies; i++) {
        sceNpTrophyGetTrophyInfo(trophy_context, handle, i, &trophy_detail, &trophy_data);
        if (only_unlockable && (trophy_detail.trophyGrade == 1 || trophy_data.unlocked)) continue;
        copy_data(&(*details)[count++], &trophy_detail, &trophy_data);
    }
    sceNpTrophyDestroyHandle(handle);
    infolog("Done.\n");
    return count;
}

int trophy_unlock(trophy_detail_t *details, int index, long int *platid, uint64_t cust_tick) {
    *platid = -1;
    SceNpTrophyHandle handle;
    int ret;
    trophy_detail_t *detail = &details[index];

    infolog("Unlocking trophy...");
    if (cust_tick) {
        FakeTimes(1);
        SetTrophyTimes(cust_tick);
    }
    sceNpTrophyCreateHandle(&handle);
    ret = sceNpTrophyUnlockTrophy(trophy_context, handle, detail->id, platid);
    sceNpTrophyDestroyHandle(handle);
    if (cust_tick)
        FakeTimes(0);
    if (ret < 0) {
        switch (ret) {
            case 0x8055160f:
                infolog("Trophy is allready unlocked.");
                break;
            case 0x80551610:
                infolog("Platinum trophy is not unlockable directly.");
                break;
            default:
                infolog("sceNpTrophyUnlockTrophy() failed. ret = 0x%x", ret);
                return -1;
        }
    } else {
        SceNpTrophyDetails trophy_detail = {0};
        SceNpTrophyData trophy_data = {0};
        infolog("Done.");
        trophy_detail.size = sizeof(SceNpTrophyDetails);
        trophy_data.size = sizeof(SceNpTrophyData);
        sceNpTrophyCreateHandle(&handle);
        sceNpTrophyGetTrophyInfo(trophy_context, handle, detail->id, &trophy_detail, &trophy_data);
        copy_data(detail, &trophy_detail, &trophy_data);
        if (platid >= 0) {
            sceNpTrophyGetTrophyInfo(trophy_context, handle, *platid, &trophy_detail, &trophy_data);
            copy_data(&detail[*platid], &trophy_detail, &trophy_data);
        }
        sceNpTrophyDestroyHandle(handle);
    }
    return 0;
}

int sort_trophy_detail(const void *v1, const void *v2) {
    const trophy_detail_t *d1 = (const trophy_detail_t *)v1;
    const trophy_detail_t *d2 = (const trophy_detail_t *)v2;
    if (d1->grade != d2->grade) {
        if (d1->grade == 0) return -1;
        if (d2->grade == 0) return 1;
        return d2->grade - d1->grade;
    }
    return d1->id - d2->id;
}

int sort_ticks(const void *v1, const void *v2) {
    const uint64_t d1 = *(const uint64_t *)v1;
    const uint64_t d2 = *(const uint64_t *)v2;
    if (d1 == d2) return 0;
    return d1 < d2 ? -1 : 1;
}

uint64_t rand64() {
    return (uint64_t)(rand() & 0xFFFF) | ((uint64_t)(rand() & 0xFFFF) << 16) | ((uint64_t)(rand() & 0xFFFF) << 32) | ((uint64_t)(rand() & 0xFFFF) << 48);
}

int trophy_unlock_all(int rand_days) {
    trophy_detail_t *details;
    SceRtcTick tick;
    SceNpTrophyHandle handle;
    uint64_t range = (uint64_t)rand_days * 86400 * (uint64_t)sceRtcGetTickResolution();
    int i;
    int count = trophy_list(&details, 1);
    if (count <= 0) {
        if (details) free(details);
        return -1;
    }
    uint64_t *ticks = (uint64_t*)calloc(count, sizeof(uint64_t));
    qsort(details, count, sizeof(trophy_detail_t), sort_trophy_detail);
    sceRtcGetCurrentTickUtc(&tick);
    srand((unsigned int)tick.tick);
    for (i = 0; i < count; ++i) {
        ticks[i] = tick.tick - (rand64() % range);
    }
    qsort(ticks, count, sizeof(uint64_t), sort_ticks);
    ticks[count - 1] = tick.tick;
    sceNpTrophyCreateHandle(&handle);
    FakeTimes(1);
    for (i = 0; i < count; ++i) {
        SceNpTrophyId platid = -1;
        int ret;
        if (details[i].unlocked || details[i].grade == 1) continue;
        infolog("Unlocking trophy %s...", details[i].id, details[i].name);
        SetTrophyTimes(ticks[i]);
        ret = sceNpTrophyUnlockTrophy(trophy_context, handle, details[i].id, &platid);
        if (ret < 0) {
            switch (ret) {
                case 0x8055160f:
                case 0x80551610:
                    break;
                default:
                    infolog("sceNpTrophyUnlockTrophy() failed for %d. ret = 0x%x", i, ret);
                    break;
            }
        }
    }
    sceNpTrophyDestroyHandle(handle);
    infolog("Done.");
    FakeTimes(0);
    free(ticks);
    free(details);
    return 0;
}
