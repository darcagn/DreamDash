#ifndef LOADER_DRAWING_H
#define LOADER_DRAWING_H

#define DRAW_FONT_WIDTH 12.0f
#define DRAW_FONT_HEIGHT 24.0f
#define DRAW_FONT_LINE_SPACING 4.0f

#define DRAW_PACK_COLOR(a, r, g, b) ( \
    a << 24 | \
    r << 16 | \
    g << 8 | \
    b << 0 )

typedef struct color_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

typedef struct rect_t {
    float left;
    float top;
    float width;
    float height;
} Rect;

typedef struct vec2_t {
    float x;
    float y;
} Vec2;

#define COL_WHITE       (Color) {255, 255, 255, 255}

#define COL_BLUE               (Color) {54, 70, 93, 255}
#define COL_BLUE_LIGHT         (Color) {178, 226, 249, 255}
#define COL_BLUE_TRANS         (Color) {54, 70, 93, 128}

#define COL_RED                (Color) {255, 81, 72, 255}
#define COL_RED_TRANS          (Color) {255, 81, 72, 64}

#define COL_YELLOW             (Color) {240, 226, 107, 255}

#define COL_TRUE_BLUE          (Color) {0, 0, 255, 255}
#define COL_TRUE_BLUE_TRANS1   (Color) {0, 0, 255, 96}
#define COL_TRUE_BLUE_TRANS2   (Color) {0, 0, 255, 128}

#define COL_GREEN              (Color) {0, 255, 0, 255}
#define COL_GREEN_TRANS        (Color) {0, 255, 0, 64}

#define COL_BLACK_TRANS1       (Color) {0, 0, 0, 128}
#define COL_BLACK_TRANS2       (Color) {0, 0, 0, 96}

void back_init();

void draw_back();

void draw_init();

void draw_exit();

void draw_start();

void draw_end();

void draw_string(float x, float y, float z, Color color, char *str);

void draw_box(float x, float y, float w, float h, float z, Color color);

void draw_box_outline(float x, float y, float w, float h, float z,
                      Color color, Color outline_color, float outline_size);

int draw_printf(int level, const char *fmt, ...);

Vec2 draw_get_screen_size();

#endif //LOADER_DRAWING_H
