#ifndef DREAMDASH_DRAWING_H
#define DREAMDASH_DRAWING_H

#include <stdint.h>

#include "color.h"

#define ROMFONT_WIDTH 	12
#define ROMFONT_HEIGHT 	24
#define DRAW_FONT_WIDTH 12.0f
#define DRAW_FONT_HEIGHT 24.0f
#define DRAW_FONT_LINE_SPACING 4.0f
#define DRAW_LINE_HEIGHT (DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING)
#define DRAW_SCREEN_HEIGHT  480
#define DRAW_SCREEN_WIDTH   640

typedef struct rect {
    float left;
    float top;
    float width;
    float height;
} rect_t;

void back_init();

void draw_back();

void draw_init();

void draw_exit();

void draw_start();

void draw_end();

void draw_string(float x, float y, float z, color_t color, char *str);

void draw_box(float x, float y, float w, float h, float z, color_t color);

void draw_box_outline(float x, float y, float w, float h, float z,
                      color_t color, color_t outline_color, float outline_size);

int draw_printf(const char *fmt, ...);

#endif //DREAMDASH_DRAWING_H
