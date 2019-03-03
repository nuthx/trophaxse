#pragma once

#include <stdint.h>

typedef struct dateedit dateedit_t;
typedef struct vita2d_pvf vita2d_pvf;

dateedit_t *dateedit_create(vita2d_pvf *font, unsigned int font_color, unsigned int sel_color, uint64_t tick);
void dateedit_destroy(dateedit_t *edit);
void dateedit_draw(dateedit_t *edit, int x, int y);
void dateedit_settick(dateedit_t *edit, uint64_t tick);
uint64_t dateedit_gettick(dateedit_t *edit);
void dateedit_move_left(dateedit_t *edit);
void dateedit_move_right(dateedit_t *edit);
void dateedit_increase(dateedit_t *edit);
void dateedit_decrease(dateedit_t *edit);
void dateedit_random(dateedit_t *edit);
