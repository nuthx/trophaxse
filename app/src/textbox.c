#include "textbox.h"

#include <vita2d.h>

#include <psp2/kernel/threadmgr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef char line_t[256];

typedef struct textbox {
    vita2d_pvf *font;
    unsigned int font_color;
    int maxline;
    int spacing;
    int count;
    line_t *lines;
    SceKernelLwMutexWork mutex;
} textbox_t;

textbox_t *textbox_create(vita2d_pvf *font, unsigned int font_color, int spacing, int maxline) {
    textbox_t *t = (textbox_t*)calloc(1, sizeof(textbox_t));
    t->font = font;
    t->font_color = font_color;
    t->maxline = maxline;
    t->spacing = spacing;
    t->lines = (line_t*)calloc(maxline, sizeof(line_t));
    sceKernelCreateLwMutex(&t->mutex, "textbox_mutex", 0, 0, NULL);
    return t;
}

void textbox_destroy(textbox_t *box) {
    if (!box) return;
    sceKernelLockLwMutex(&box->mutex, 1, NULL);
    if (box->lines) free(box->lines);
    sceKernelUnlockLwMutex(&box->mutex, 1);
    sceKernelDeleteLwMutex(&box->mutex);
    free(box);
}

int textbox_clear(textbox_t *box) {
    box->count = 0;
    return 0;
}

int textbox_count(const textbox_t *box) {
    return box->count;
}

int textbox_add(textbox_t *box, const char *str) {
    line_t *n;
    int res;
    sceKernelLockLwMutex(&box->mutex, 1, NULL);
    if (box->count >= box->maxline) {
        memmove(&box->lines[0], box->lines[1], (box->maxline - 1) * sizeof(line_t));
        n = &box->lines[box->maxline - 1];
    } else {
        n = &box->lines[box->count++];
    }
    snprintf(*n, 256, "%s", str);
    res = box->count - 1;
    sceKernelUnlockLwMutex(&box->mutex, 1);
    return res;
}

int textbox_addf(textbox_t *box, const char *str, ...) {
    char buf[1024];
    va_list argptr;
    va_start(argptr, str);
    vsnprintf(buf, sizeof(buf), str, argptr);
    va_end(argptr);
    return textbox_add(box, buf);
}

int textbox_draw(textbox_t *box, int x, int y) {
    int i;
    sceKernelLockLwMutex(&box->mutex, 1, NULL);
    for (i = 0; i < box->count; ++i) {
        vita2d_pvf_draw_text(box->font, x, y, box->font_color, 1.0f, box->lines[i]);
        y += box->spacing;
    }
    sceKernelUnlockLwMutex(&box->mutex, 1);
    return 0;
}
