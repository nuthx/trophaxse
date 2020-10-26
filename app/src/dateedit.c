#include "dateedit.h"

#include <vita2d.h>
#include <psp2/rtc.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct dateedit {
    vita2d_pvf *pvf;
    unsigned int font_color;
    unsigned int sel_color;
    SceDateTime time;
    int pos;
} dateedit_t;

dateedit_t *dateedit_create(vita2d_pvf *font, unsigned int font_color, unsigned int sel_color, uint64_t tick) {
    dateedit_t *edit = (dateedit_t*)calloc(1, sizeof(dateedit_t));
    edit->pvf = font;
    edit->font_color = font_color;
    edit->sel_color = sel_color;
    dateedit_settick(edit, tick);
    return edit;
}

extern uint64_t rand64();

void dateedit_destroy(dateedit_t *edit) {
    free(edit);
}

void dateedit_draw(dateedit_t *edit, int x, int y) {
    char v[8];
    snprintf(v, 8, "%04d", edit->time.year);
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->pos == 0 ? edit->sel_color : edit->font_color, 1.0f, v);
    x+=48;
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->font_color, 1.0f, "/");
    x+=12;
    snprintf(v, 8, "%02d", edit->time.month);
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->pos == 1 ? edit->sel_color : edit->font_color, 1.0f, v);
    x+=24;
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->font_color, 1.0f, "/");
    x+=12;
    snprintf(v, 8, "%02d", edit->time.day);
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->pos == 2 ? edit->sel_color : edit->font_color, 1.0f, v);
    x+=36;
    snprintf(v, 8, "%02d", edit->time.hour);
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->pos == 3 ? edit->sel_color : edit->font_color, 1.0f, v);
    x+=27;
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->font_color, 1.0f, ":");
    x+=9;
    snprintf(v, 8, "%02d", edit->time.minute);
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->pos == 4 ? edit->sel_color : edit->font_color, 1.0f, v);
    x+=27;
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->font_color, 1.0f, ":");
    x+=9;
    snprintf(v, 8, "%02d", edit->time.second);
    vita2d_pvf_draw_text(edit->pvf, x, y, edit->pos == 5 ? edit->sel_color : edit->font_color, 1.0f, v);
}

void dateedit_settick(dateedit_t *edit, uint64_t tick) {
    if (tick == 0) {
        sceRtcGetCurrentClockLocalTime(&edit->time);
    } else {
        SceRtcTick t = {tick};
        sceRtcConvertUtcToLocalTime(&t, &t);
        sceRtcConvertTickToDateTime(&t, &edit->time);
    }
    edit->time.microsecond = 0;
}

uint64_t dateedit_gettick(dateedit_t *edit) {
    SceRtcTick t;
    edit->time.microsecond = 0;
    sceRtcConvertDateTimeToTick(&edit->time, &t);
    sceRtcConvertLocalTimeToUtc(&t, &t);
    return t.tick;
}

void dateedit_move_left(dateedit_t *edit) {
    if (--edit->pos < 0) edit->pos = 5;
}

void dateedit_move_right(dateedit_t *edit) {
    if (++edit->pos > 5) edit->pos = 0;
}

void dateedit_increase(dateedit_t *edit) {
    switch(edit->pos) {
        case 0:
            edit->time.year++;
            if (sceRtcCheckValid(&edit->time)) edit->time.year = 2010;
            break;
        case 1:
            edit->time.month++;
            if (sceRtcCheckValid(&edit->time)) edit->time.month = 1;
            break;
        case 2:
            edit->time.day++;
            if (sceRtcCheckValid(&edit->time)) edit->time.day = 1;
            break;
        case 3:
            edit->time.hour++;
            if (sceRtcCheckValid(&edit->time)) edit->time.hour = 0;
            break;
        case 4:
            edit->time.minute++;
            if (sceRtcCheckValid(&edit->time)) edit->time.minute = 0;
            break;
        case 5:
            edit->time.second++;
            if (sceRtcCheckValid(&edit->time)) edit->time.second = 0;
            break;
        default: break;
    }
}

void dateedit_decrease(dateedit_t *edit) {
    switch(edit->pos) {
        case 0:
            edit->time.year--;
            if (sceRtcCheckValid(&edit->time)) edit->time.year = 2020;
            break;
        case 1:
            edit->time.month--;
            if (sceRtcCheckValid(&edit->time)) edit->time.month = 12;
            break;
        case 2:
            edit->time.day--;
            if (sceRtcCheckValid(&edit->time)) {
                edit->time.day = 31;
                while (sceRtcCheckValid(&edit->time))
                    edit->time.day--;
            }
            break;
        case 3:
            edit->time.hour--;
            if (sceRtcCheckValid(&edit->time)) edit->time.hour = 23;
            break;
        case 4:
            edit->time.minute--;
            if (sceRtcCheckValid(&edit->time)) edit->time.minute = 59;
            break;
        case 5:
            edit->time.second--;
            if (sceRtcCheckValid(&edit->time)) edit->time.second = 59;
            break;
        default: break;
    }
}

void dateedit_random(dateedit_t *edit) {
    SceRtcTick t;
    uint64_t reso = sceRtcGetTickResolution();
    sceRtcGetCurrentTickUtc(&t);
    srand((unsigned int)t.tick);
    t.tick -= rand64() % (reso * 86400ULL * 365ULL);
    dateedit_settick(edit, t.tick);
}
