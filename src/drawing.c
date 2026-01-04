#include <stdarg.h>
#include <string.h>

#include <dc/biosfont.h>

#include <png/png.h>

#include "bmfont.h"
#include "drawing.h"

static pvr_ptr_t font_tex = NULL;

pvr_init_params_t params = {
        {PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0},
        512 * 1024
};

typedef struct {
    char id[4];
    short width;
    short height;
    int type;
    int size;
} tex_header_t;

pvr_ptr_t back_tex;

void back_init(void) {
    back_tex = pvr_mem_malloc(WALLPAPER_WIDTH * WALLPAPER_HEIGHT * 2);
    png_to_texture("/rd/"WALLPAPER_FILE, back_tex, PNG_NO_ALPHA);
}

void draw_back(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;

    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, WALLPAPER_WIDTH, WALLPAPER_HEIGHT, back_tex, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;

    vert.x = 1;
    vert.y = 1;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 640;
    vert.y = 1;
    vert.z = 1;
    vert.u = 1.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 1;
    vert.y = 480;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 1.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 640;
    vert.y = 480;
    vert.z = 1;
    vert.u = 1.0;
    vert.v = 1.0;
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));
}

#ifndef ROMFONT
static BMFont bmf_font;
#endif

static void draw_init_font() {
#ifdef ROMFONT
    /* Draw biosfont to a texture in vram */
    uint16_t *vram;
    int x, y;

    font_tex = pvr_mem_malloc(256*256*2);
    vram = (uint16_t *)font_tex;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 16; x++) {
            bfont_draw(vram, 256, 0, y*16 + x);
            vram += 16;
        }
        vram += 23*256;
    }
#else
    FILE *fp;
    tex_header_t hdr;

    // parse BMFont font information
    if (bmf_parse("/rd/"BMFONT_NAME".fnt", &bmf_font) != 0) {
        return;
    }

    // load "texconv" texture
    fp = fopen("/rd/"BMFONT_NAME".tex", "r");
    if (fp == NULL) {
        return;
    }
    // read "texconv" texture header
    fread(&hdr, sizeof(hdr), 1, fp);
    // allocate pvr mem
    font_tex = pvr_mem_malloc(hdr.size);
    // read "texconv" texture to pvr mem
    fread(font_tex, hdr.size, 1, fp);

    // all done
    fclose(fp);
#endif
}

static size_t draw_char(float x1, float y1, float z1, color_t color, int c) {
#ifdef ROMFONT
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
#else
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
#endif
}

/* draw len chars at string */
void draw_string(float x, float y, float z, color_t color, char *str) {
    int i, len;
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t poly;

#ifdef ROMFONT
    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED,
#else
    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_VQ_ENABLE,
#endif
                     256, 256, font_tex, PVR_FILTER_NONE);
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

void draw_init() {
    pvr_init(&params);
    draw_init_font();
}

void draw_exit() {
    if (font_tex != NULL) {
        pvr_mem_free(font_tex);
    }
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
    if (font_tex == NULL) {
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
