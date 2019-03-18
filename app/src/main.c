#include "trophy.h"
#include "listbox.h"
#include "textbox.h"
#include "dateedit.h"

#include <vita2d.h>

#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/rtc.h>
#include <psp2/sysmodule.h>
#include <psp2/system_param.h>

#include <psp2/io/fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

static int is_latin(unsigned int c) {
    return c < 0x2000;
}

typedef enum state {
    state_none = 0,
    state_main_menu = 1,
    state_unlock = 2,
    state_trophy = 3,
    state_date = 4,
} state_t;

static state_t state = state_none;
static char title_text[128] = {};
static listbox_t *box = NULL;
static textbox_t *text = NULL;
static dateedit_t *date = NULL;
static int games_count = 0;
static trophy_gameinfo_t *games = NULL;
static int trophy_count = 0;
static trophy_detail_t *trophies = NULL;
static char game_titleid[16];
static int curr_game = -1;
static int cust_time = 0;
static uint64_t last_cust_tick = 0ULL;
static int worker_type = -1;
static int worker_id = -1;
static int worker_param = -1;
static int worker_result = -1;

int infolog(const char *str, ...) {
    char buf[1024];
    va_list argptr;
    va_start(argptr, str);
    vsnprintf(buf, sizeof(buf), str, argptr);
    va_end(argptr);
    return textbox_add(text, buf);
}

int debuglog(const char *str, ...) {
    char buf[1024];
    va_list argptr;
    va_start(argptr, str);
    int size = vsnprintf(buf, sizeof(buf), str, argptr);
    va_end(argptr);
    SceUID fd = sceIoOpen("ux0:temp/debug.log", SCE_O_RDWR | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd < 0)
        return fd;
    sceIoLseek(fd, 0, SCE_SEEK_END);
    int written = sceIoWrite(fd, buf, size);
    sceIoWrite(fd, "\n", 1);
    sceIoClose(fd);
    return written;
}

void main_menu() {
    int i;
    snprintf(title_text, 128, "**** TropHax StandAlone Edition MOD ****");
    curr_game = -1;
    listbox_clear(box);
    for (i = 0; i < games_count; ++i) {
        char name[256];
        snprintf(name, 256, "[%s] %s\n", games[i].trophyid, games[i].name);
        listbox_add(box, name, "");
    }
    state = state_main_menu;
}

void unlock_menu() {
    snprintf(title_text, 128, "[%s][%s] %s", game_titleid, games[curr_game].trophyid, games[curr_game].name);
    listbox_clear(box);
    listbox_add(box, "Unlock Single", "");
    listbox_add(box, "Unlock Single (custom date/time)", "");
    listbox_add(box, "Unlock All (Random ~5 days)", "");
    listbox_add(box, "Unlock All (Random ~15 days)", "");
    listbox_add(box, "Unlock All (Random ~30 days)", "");
    listbox_add(box, "Unlock All (Random ~90 days)", "");
    listbox_add(box, "Unlock All (Random ~180 days)", "");
    state = state_unlock;
}

void trophy_list_menu() {
    int i;
    listbox_clear(box);
    for (i = 0; i < trophy_count; ++i) {
        listbox_add(box, trophies[i].name, trophies[i].description);
    }
    state = state_trophy;
}

int worker_trophy_prepare(SceSize args, void *argp) {
    worker_result = trophy_prepare(games[worker_param].trophyid, game_titleid);
    worker_id = 0;
    return sceKernelExitDeleteThread(0);
}

int worker_trophy_list(SceSize args, void *argp) {
    if (trophies != NULL) { free(trophies); trophies = NULL; }
    trophy_count = trophy_list(&trophies, 0);
    worker_result = 0;
    worker_id = 0;
    return sceKernelExitDeleteThread(0);
}

int worker_trophy_unlock(SceSize args, void *argp) {
    long int platid = -1;
    worker_result = trophy_unlock(trophies, worker_param, &platid, cust_time ? last_cust_tick : 0ULL);
    worker_param |= ((int)platid+1) << 8;
    worker_id = 0;
    return sceKernelExitDeleteThread(0);
}

int worker_trophy_unlock_all(SceSize args, void *argp) {
    worker_result = trophy_unlock_all(worker_param, cust_time ? last_cust_tick : 0ULL);
    worker_id = 0;
    return sceKernelExitDeleteThread(0);
}

int process_ok() {
    switch (state) {
        case state_main_menu: {
            int sel = listbox_sel(box);
            if (sel < 0) break;
            worker_type = 0;
            worker_param = sel;
            worker_id = sceKernelCreateThread("worker_trophy_prepare", worker_trophy_prepare, 0x10000100, 0x10000, 0, 0, NULL);
            sceKernelStartThread(worker_id, 0, 0);
            break;
        }
        case state_unlock: {
            int sel = listbox_sel(box);
            switch (sel) {
                case 0:
                case 1: {
                    cust_time = sel;
                    worker_type = 1;
                    worker_id = sceKernelCreateThread("worker_trophy_list", worker_trophy_list, 0x10000100, 0x10000, 0, 0, NULL);
                    sceKernelStartThread(worker_id, 0, 0);
                    break;
                }
                default: {
                    cust_time = sel;
                    state = state_date;
                    break;
                }
            }
            break;
        }
        case state_trophy: {
            int sel = listbox_sel(box);
            if (sel < 0) break;
            if (cust_time) {
                dateedit_settick(date, last_cust_tick);
                state = state_date;
                break;
            }
            worker_type = 2;
            worker_param = sel;
            worker_id = sceKernelCreateThread("worker_trophy_unlock", worker_trophy_unlock, 0x10000100, 0x10000, 0, 0, NULL);
            sceKernelStartThread(worker_id, 0, 0);
            break;
        }
        case state_date: {
            int sel = listbox_sel(box);
            if (sel < 0) break;
            last_cust_tick = dateedit_gettick(date);
            if (cust_time == 1) {
                worker_type = 2;
                worker_param = sel;
                worker_id = sceKernelCreateThread("worker_trophy_unlock", worker_trophy_unlock, 0x10000100, 0x10000, 0, 0, NULL);
                sceKernelStartThread(worker_id, 0, 0);
                state = state_trophy;
            } else {
                const int rand_days[] = {0, 0, 5, 15, 30, 90, 180};
                worker_type = 3;
                worker_param = rand_days[cust_time];
                worker_id = sceKernelCreateThread("worker_trophy_unlock_all", worker_trophy_unlock_all, 0x10000100, 0x10000, 0, 0, NULL);
                sceKernelStartThread(worker_id, 0, 0);
                state = state_unlock;
            }
            break;
        }
        default: break;
    }
    return 0;
}

int process_cancel() {
    switch (state) {
        case state_unlock: {
            trophy_finish();
            main_menu();
            break;
        }
        case state_trophy: {
            unlock_menu();
            break;
        }
        case state_date: {
            last_cust_tick = dateedit_gettick(date);
            state = state_trophy;
            break;
        }
        default: break;
    }
    return 0;
}

int main() {
    SceCtrlData pad;
    unsigned int last_button = 0;
    uint64_t inter = sceRtcGetTickResolution() / 5, rep_inter = inter / 4;
    SceRtcTick tick;
    int repeating = 0;
    vita2d_pvf *pvf;
    int enter_button = 0;
    unsigned int ok_button, cancel_button;

    sceSysmoduleLoadModule(SCE_SYSMODULE_NP);
    sceSysmoduleLoadModule(SCE_SYSMODULE_NP_TROPHY);

    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter_button);
    if (enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE) {
        ok_button = SCE_CTRL_CIRCLE;
        cancel_button = SCE_CTRL_CROSS;
    } else {
        ok_button = SCE_CTRL_CROSS;
        cancel_button = SCE_CTRL_CIRCLE;
    }
    vita2d_system_pvf_config pvf_conf[2] = {{SCE_PVF_DEFAULT_LANGUAGE_CODE, is_latin}, {SCE_PVF_LANGUAGE_C, NULL}};

    vita2d_init();
    vita2d_set_clear_color(RGBA8(0x20, 0x20, 0x20, 0xFF));

    pvf = vita2d_load_system_pvf(2, pvf_conf);

    box = listbox_create(pvf, RGBA8(180,180,180,255), RGBA8(80,255,80,255), 20, 15);
    text = textbox_create(pvf, RGBA8(180,180,180,255), 20, 9);
    date = dateedit_create(pvf, RGBA8(180,180,180,255), RGBA8(80,255,80,255), 0ULL);

    games_count = trophy_games_list(&games);
    main_menu();

    memset(&pad, 0, sizeof(pad));
    sceRtcGetCurrentTickUtc(&tick);
    while (1) {
        if (worker_id == 0) {
            if (worker_result == 0) {
                switch(worker_type) {
                    case 0:
                        curr_game = worker_param;
                        unlock_menu();
                        break;
                    case 1:
                        trophy_list_menu();
                        break;
                    case 2: {
                        int index = worker_param & 0xFF;
                        listbox_set(box, index, trophies[index].name, trophies[index].description);
                        if (worker_param >= 0x100) {
                            int platid = (worker_param >> 8) - 1;
                            int i;
                            for (i = 0; i < trophy_count; ++i) {
                                if (trophies[i].id == platid) {
                                    listbox_set(box, i, trophies[i].name, trophies[i].description);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    default: break;
                }
            }
            worker_type = -1;
            worker_id = -1;
            worker_param = -1;
            worker_result = -1;
        } else if (worker_id < 0) {
            SceRtcTick curr;
            int diff;
            sceCtrlPeekBufferPositiveExt2(0, &pad, 1);

            sceRtcGetCurrentTickUtc(&curr);
            diff = pad.buttons != last_button;
            if (diff || curr.tick - tick.tick >= (repeating ? rep_inter : inter)) {
                repeating = !diff;
                last_button = pad.buttons;
                tick = curr;
                if (pad.buttons & SCE_CTRL_START)
                    break;
                if (pad.buttons & SCE_CTRL_SELECT) {
                    textbox_clear(text);
                }
                if (pad.buttons & ok_button) {
                    if (process_ok()) break;
                }
                if (pad.buttons & cancel_button) {
                    if (process_cancel()) break;
                }

                if (state == state_date) {
                    switch (pad.buttons & (SCE_CTRL_LEFT | SCE_CTRL_RIGHT | SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_TRIANGLE)) {
                        case SCE_CTRL_LEFT: dateedit_move_left(date); break;
                        case SCE_CTRL_RIGHT: dateedit_move_right(date); break;
                        case SCE_CTRL_UP: dateedit_increase(date); break;
                        case SCE_CTRL_DOWN: dateedit_decrease(date); break;
                        case SCE_CTRL_TRIANGLE: dateedit_random(date); break;
                    }
                } else {
                    switch (pad.buttons & (SCE_CTRL_LEFT | SCE_CTRL_RIGHT | SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_L1 | SCE_CTRL_R1)) {
                        case SCE_CTRL_LEFT: listbox_page_up(box); break;
                        case SCE_CTRL_RIGHT: listbox_page_down(box); break;
                        case SCE_CTRL_UP: listbox_move_up(box); break;
                        case SCE_CTRL_DOWN: listbox_move_down(box); break;
                        case SCE_CTRL_L1: listbox_move_begin(box); break;
                        case SCE_CTRL_R1: listbox_move_end(box); break;
                    }
                }
            }
        }
        vita2d_start_drawing();
        vita2d_clear_screen();

        vita2d_draw_line(0, 332, 964, 332, RGBA8(200, 200, 200, 255));
        vita2d_draw_line(0, 355, 964, 355, RGBA8(200, 200, 200, 255));
        vita2d_pvf_draw_text(pvf, 20, 25, RGBA8(225, 225, 225, 255), 1.f, title_text);
        listbox_draw(box, 20, 48, 20, 350);
        textbox_draw(text, 20, 374);
        if (state == state_date) {
            vita2d_draw_rectangle(100, 100, 240, 27, RGBA8(0x50, 0x50, 0x50, 0xFF));
            dateedit_draw(date, 105, 120);
        }

        vita2d_end_drawing();
        vita2d_swap_buffers();
    }
    if (trophies) { free(trophies); trophies = NULL; trophy_count = 0; }
    if (games) { free(games); games = NULL; games_count = 0; }

    dateedit_destroy(date);
    textbox_destroy(text);
    listbox_destroy(box);

    vita2d_fini();
    vita2d_free_pvf(pvf);

    sceKernelExitProcess(0);
    return 0;
}
