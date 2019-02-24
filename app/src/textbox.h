#pragma once

typedef struct textbox textbox_t;
typedef struct vita2d_pvf vita2d_pvf;

textbox_t *textbox_create(vita2d_pvf *font, unsigned int font_color, int spacing, int maxline);
void textbox_destroy(textbox_t *box);
int textbox_clear(textbox_t *box);
int textbox_count(const textbox_t *box);
int textbox_add(textbox_t *box, const char *str);
int textbox_addf(textbox_t *box, const char *str, ...);
int textbox_draw(textbox_t *box, int x, int y);
