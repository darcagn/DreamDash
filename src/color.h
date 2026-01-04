#ifndef DREAMDASH_COLOR_H
#define DREAMDASH_COLOR_H

#define DRAW_PACK_COLOR(a, r, g, b) ( \
    a << 24 | \
    r << 16 | \
    g << 8 | \
    b << 0 )

typedef struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} color_t;

#define COL_WHITE              (color_t) { 255, 255, 255, 255 }
#define COL_BLUE               (color_t) {  54,  70,  93, 255 }
#define COL_BLUE_LIGHT         (color_t) { 178, 226, 249, 255 }
#define COL_TRUE_BLUE          (color_t) {   0,   0, 255, 255 }
#define COL_RED                (color_t) { 255,  81,  72, 255 }
#define COL_YELLOW             (color_t) { 240, 226, 107, 255 }
#define COL_GREEN              (color_t) {   0, 255,   0, 255 }
#define COL_BLACK              (color_t) {   0,   0,   0, 255 }

#define COL_ALPHA(col, alpha) \
    (color_t) {(col).r, (col).g, (col).b, (alpha)}

#endif //DREAMDASH_COLOR_H
