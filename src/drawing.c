#include <stdarg.h>
#include <string.h>

#include <kos/dbglog.h>

#include <dc/biosfont.h>
#include <dc/pvr.h>

#include "bmfont.h"
#include "drawing.h"
#include "texture.h"

static pvr_texture_t font_tex;

#ifdef ROMFONT
static void draw_init_font(void) {
    /* Draw biosfont to a texture in vram */
    texture_load_romfont(&font_tex);
    if(!texture_valid(&font_tex)) {
        dbglog(DBG_ERROR, "Error initting font!\n");
    }
}

static size_t draw_char(float x1, float y1, float z1, color_t color, int c) {
    pvr_vertex_t vert;
    int ix, iy;
    float u1, v1, u2, v2;

    ix = (c % 16) * 16;
    iy = (c / 16) * 24;
    u1 = ix * 1.0f / 256.0f;
    v1 = iy * 1.0f / 256.0f;
    u2 = (ix+12) * 1.0f / 256.0f;
    v2 = (iy+24) * 1.0f / 256.0f;

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x1;
    vert.y = y1 + ROMFONT_HEIGHT;
    vert.z = z1;
    vert.u = u1;
    vert.v = v2;
    vert.argb = DRAW_PACK_COLOR(color.a, color.r, color.g, color.b);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1;
    vert.y = y1;
    vert.u = u1;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1 + ROMFONT_WIDTH;
    vert.y = y1 + ROMFONT_HEIGHT;
    vert.u = u2;
    vert.v = v2;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x1 + ROMFONT_WIDTH;
    vert.y = y1;
    vert.u = u2;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));

    return ROMFONT_WIDTH;
}
#else // ROMFONT
static BMFont bmf_font;

static void draw_init_font(void) {
    /* Parse BMFont font information */
    if (bmf_parse("/rd/font.fnt", &bmf_font) != 0) {
        dbglog(DBG_INFO, "couldn't load font info, uh oh\n");
        return;
    }

    texture_load(&font_tex, "/rd/font.pvr");
    if(!texture_valid(&font_tex)) {
        return;
    }
}

static size_t draw_char(float x1, float y1, float z1, color_t color, int c) {
    pvr_vertex_t vert;

    BMFontChar *bmf_char = &bmf_font.chars[c];

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x1 + (float) bmf_char->xoffset;
    vert.y = y1 + (float) bmf_char->height + (float) bmf_char->yoffset;
    vert.z = z1;
    vert.u = (float) bmf_char->x / (float) bmf_font.common.scaleW;
    vert.v = (float) (bmf_char->y + bmf_char->height) / (float) bmf_font.common.scaleH;
    vert.argb = DRAW_PACK_COLOR(color.a, color.r, color.g, color.b);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1 + (float) bmf_char->xoffset;
    vert.y = y1 + (float) bmf_char->yoffset;
    vert.u = (float) bmf_char->x / (float) bmf_font.common.scaleW;
    vert.v = (float) bmf_char->y / (float) bmf_font.common.scaleH;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1 + (float) (bmf_char->width + bmf_char->xoffset);
    vert.y = y1 + (float) (bmf_char->height + bmf_char->yoffset);
    vert.u = (float) (bmf_char->x + bmf_char->width) / (float) bmf_font.common.scaleW;
    vert.v = (float) (bmf_char->y + bmf_char->height) / (float) bmf_font.common.scaleH;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x1 + (float) (bmf_char->width + bmf_char->xoffset);
    vert.y = y1 + (float) bmf_char->yoffset;
    vert.u = (float) (bmf_char->x + bmf_char->width) / (float) bmf_font.common.scaleW;
    vert.v = (float) bmf_char->y / (float) bmf_font.common.scaleH;
    pvr_prim(&vert, sizeof(vert));

    return (float) (bmf_char->xadvance + bmf_char->xoffset);
}
#endif // ROMFONT

/* draw len chars at string */
void draw_string(float x, float y, float z, color_t color, char *str) {
    if(!str || !texture_valid(&font_tex)) {
        return;
    }

    int i, len;
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t poly;

    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, font_tex.format,
                     font_tex.width, font_tex.height, font_tex.buffer, PVR_FILTER_NONE);
    pvr_poly_compile(&poly, &cxt);
    pvr_prim(&poly, sizeof(poly));

    len = strlen(str);
    for (i = 0; i < len; i++) {
        char c = str[i];
        if (!(c > 31 && c < 127)) {
            continue;
        }
        x += draw_char(x, y, z, color, c);
    }
}

/* draw a box (used by cursor and border, etc) (at 1.0f z coord) */
void draw_box(float x, float y, float w, float h, float z, color_t color) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t poly;
    pvr_vertex_t vert;

    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    pvr_poly_compile(&poly, &cxt);
    pvr_prim(&poly, sizeof(poly));

    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;
    vert.y = y + h;
    vert.z = z;
    vert.u = vert.v = 0.0f;
    vert.argb = DRAW_PACK_COLOR(color.a, color.r, color.g, color.b);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.y -= h;
    pvr_prim(&vert, sizeof(vert));

    vert.y += h;
    vert.x += w;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.y -= h;
    pvr_prim(&vert, sizeof(vert));
}

void draw_box_outline(float x, float y, float w, float h, float z, color_t color,
                      color_t outline_color, float outline_size) {

    draw_box(x - outline_size, y - outline_size, w + (outline_size * 2), h + (outline_size * 2), z - 1, outline_color);
    draw_box(x, y, w, h, z, color);
}

void draw_rect(rect_t rect, float z, color_t color) {
    draw_box(rect.left, rect.top, rect.width, rect.height, z, color);
}

void draw_rect_outline(rect_t rect, float z, color_t color,
                       color_t outline_color, float outline_size) {
    draw_box_outline(rect.left, rect.top, rect.width, rect.height, z, color, outline_color, outline_size);
}

void draw_string_rect(rect_t rect, float z, color_t color, char *str) {
    draw_string(rect.left + 5,
                rect.top + DRAW_FONT_LINE_SPACING,
                z, color, str);
}

void draw_string_rect_line(rect_t rect, float z, color_t color, char *str, size_t line) {
    draw_string(rect.left + 5,
                rect.top + DRAW_FONT_LINE_SPACING + (float)((line - 1) * DRAW_LINE_HEIGHT),
                z, color, str);
}

void draw_init() {
    pvr_init_defaults();
    draw_init_font();
}

void draw_exit() {
    texture_free(&font_tex);
}

void draw_start() {
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);
}

void draw_end() {
    pvr_list_finish();
    pvr_scene_finish();
}

int draw_printf(const char *fmt, ...) {
    if (!texture_valid(&font_tex)) {
        return 0;
    }

    char buff[512];
    va_list args;
    color_t color = COL_WHITE;

    memset(buff, 0, 512);
    va_start(args, fmt);
    int ret = vsnprintf(buff, 512, fmt, args);
    va_end(args);

    draw_start();
    draw_string(16, DRAW_SCREEN_HEIGHT - DRAW_FONT_HEIGHT - 16, 200, color, buff);
    draw_end();

    return ret;
}
