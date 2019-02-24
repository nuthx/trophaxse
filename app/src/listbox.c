#include "listbox.h"

#include <vita2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct listbox_item {
    char name[256];
    char desc[256];
} listbox_item_t;

typedef struct listbox {
    int count;
    int cap;
    int sel;
    int top;
    listbox_item_t *items;
    vita2d_pvf *font;
    unsigned int font_color;
    unsigned int sel_color;
    int spacing;
    int page_count;
} listbox_t;

listbox_t *listbox_create(vita2d_pvf *font, unsigned int font_color, unsigned int sel_color, int spacing, int page_count) {
    listbox_t *m = (listbox_t*)calloc(1, sizeof(listbox_t));
    m->font = font;
    m->font_color = font_color;
    m->sel_color = sel_color;
    m->spacing = spacing;
    m->page_count = page_count;
    return m;
}

void listbox_destroy(listbox_t *listbox) {
    if (!listbox) return;
    if (listbox->items) free(listbox->items);
    free(listbox);
}

void listbox_clear(listbox_t *listbox) {
    listbox->count = 0;
    listbox->top = 0;
    listbox->sel = 0;
}

int listbox_count(const listbox_t *listbox) {
    return listbox->count;
}

int listbox_add(listbox_t *listbox, const char *name, const char *desc) {
    listbox_item_t *item;
    if (listbox->count >= listbox->cap) {
        listbox->cap += 8;
        listbox->items = (listbox_item_t*)realloc(listbox->items, sizeof(listbox_item_t) * listbox->cap);
    }
    item = &listbox->items[listbox->count++];
    snprintf(item->name, 256, "%s", name);
    snprintf(item->desc, 256, "%s", desc);
    return listbox->count - 1;
}

int listbox_set(listbox_t *listbox, int index, const char *name, const char *desc) {
    listbox_item_t *item;
    if (index < 0 || index >= listbox->count) return -1;
    item = &listbox->items[index];
    snprintf(item->name, 256, "%s", name);
    snprintf(item->desc, 256, "%s", desc);
    return 0;
}

int listbox_insert(listbox_t *listbox, int index, const char *name, const char *desc) {
    listbox_item_t *item;
    if (index >= listbox->count) return listbox_add(listbox, name, desc);
    if (listbox->count >= listbox->cap) {
        listbox->cap += 8;
        listbox->items = (listbox_item_t*)realloc(listbox->items, sizeof(listbox_item_t) * listbox->cap);
    }
    memmove(&listbox->items[index+1], &listbox->items[index], sizeof(listbox_item_t) * (listbox->count - index));
    listbox->count++;
    item = &listbox->items[index];
    snprintf(item->name, 256, "%s", name);
    snprintf(item->desc, 256, "%s", desc);
    return 0;
}

int listbox_set_sel(listbox_t *listbox, int sel) {
    if (sel >= 0 && sel < listbox->count)
        listbox->sel = sel;
    else
        listbox->sel = -1;
    return listbox->sel;
}

int listbox_sel(const listbox_t *listbox) {
    return listbox->sel;
}

int listbox_move_up(listbox_t *listbox) {
    --listbox->sel;
    if (listbox->sel < 0) {
        return listbox_move_end(listbox);
    } else {
        if (listbox->sel < listbox->top) listbox->top = listbox->sel;
    }
    return listbox->sel;
}

int listbox_move_down(listbox_t *listbox) {
    ++listbox->sel;
    if (listbox->sel >= listbox->count) {
        return listbox_move_begin(listbox);
    } else {
        if (listbox->top + listbox->page_count <= listbox->sel)
            listbox->top = listbox->sel - listbox->page_count + 1;
    }
    return listbox->sel;
}

int listbox_move_begin(listbox_t *listbox) {
    listbox->sel = 0;
    listbox->top = 0;
    return listbox->sel;
}

int listbox_move_end(listbox_t *listbox) {
    listbox->sel = listbox->count - 1;
    if (listbox->top + listbox->page_count <= listbox->sel)
        listbox->top = listbox->sel - listbox->page_count + 1;
    return listbox->sel;
}

int listbox_page_up(listbox_t *listbox) {
    listbox->sel -= listbox->page_count;
    if (listbox->sel < 0) listbox->sel = 0;
    if (listbox->sel < listbox->top) listbox->top = listbox->sel;
    return listbox->sel;
}

int listbox_page_down(listbox_t *listbox) {
    listbox->sel += listbox->page_count;
    if (listbox->sel >= listbox->count) listbox->sel = listbox->count - 1;
    if (listbox->top + listbox->page_count <= listbox->sel)
        listbox->top = listbox->sel - listbox->page_count + 1;
    return listbox->sel;
}

int listbox_draw(listbox_t *listbox, int x, int y, int desc_x, int desc_y) {
    int i = listbox->top, e = i + listbox->page_count;
    if (e > listbox->count) e = listbox->count;
    for (; i < e; ++i) {
        if (i == listbox->sel) {
    		vita2d_pvf_draw_text(listbox->font, x, y, listbox->sel_color, 1.0f, listbox->items[i].name);
    		if (desc_x >= 0) vita2d_pvf_draw_text(listbox->font, desc_x, desc_y, listbox->font_color, 1.0f, listbox->items[i].desc);
        } else
    		vita2d_pvf_draw_text(listbox->font, x, y, listbox->font_color, 1.0f, listbox->items[i].name);
        y += listbox->spacing;
    }
    return 0;
}
