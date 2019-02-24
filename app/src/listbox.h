#pragma once

typedef struct listbox listbox_t;
typedef struct vita2d_pvf vita2d_pvf;

listbox_t *listbox_create(vita2d_pvf *font, unsigned int font_color, unsigned int sel_color, int spacing, int page_count);
void listbox_destroy(listbox_t *listbox);
void listbox_clear(listbox_t *listbox);
int listbox_count(const listbox_t *listbox);
int listbox_add(listbox_t *listbox, const char *name, const char *desc);
int listbox_set(listbox_t *listbox, int index, const char *name, const char *desc);
int listbox_insert(listbox_t *listbox, int index, const char *name, const char *desc);
int listbox_draw(listbox_t *listbox, int x, int y, int desc_x, int desc_y);
int listbox_set_sel(listbox_t *listbox, int sel);
int listbox_sel(const listbox_t *listbox);
int listbox_move_up(listbox_t *listbox);
int listbox_move_down(listbox_t *listbox);
int listbox_move_begin(listbox_t *listbox);
int listbox_move_end(listbox_t *listbox);
int listbox_page_up(listbox_t *listbox);
int listbox_page_down(listbox_t *listbox);
