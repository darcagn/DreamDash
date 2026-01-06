#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kos/dbglog.h>

#include <dc/biosfont.h>

#include "texture.h"

/* PVR file header with PVRT magic word */
typedef struct __attribute__((packed)) pvrt_header {
    uint8_t magic[4];
    uint32_t size;
    uint8_t pixel_format;
    uint8_t data_format;
    uint16_t reserved;
    uint16_t width;
    uint16_t height;
} pvr_header_t;

static size_t texture_fill_from_buffer(pvr_texture_t *texture, uint8_t *buffer, size_t size) {
    /* Clear texture */
    *texture = (pvr_texture_t) {
        .buffer = NULL,
        .width = 0,
        .height = 0,
        .format = 0,
    };

    pvr_header_t *header = NULL;

    /* Search for PVR header in the first 64 bytes at 4 byte increments */
    for (size_t i = 0; i < 64; i += 4) {
        if (buffer[i] == 'P' && buffer[i+1] == 'V' &&
            buffer[i+2] == 'R' && buffer[i+3] == 'T') {
        header = (pvr_header_t *)(buffer + i);
        break;
        }
    }

    if (!header) {
        dbglog(DBG_ERROR, "Error finding PVR header in %s!\n", __func__);
        return 0;
    }

    /* Determine texture pixel color format */
    uint32_t tex_color;
    switch (header->pixel_format) {
        case 0x00:  /* ARGB1555 (1-bit alpha, 5-bit RGB) */
            tex_color = PVR_TXRFMT_ARGB1555;
            break;

        case 0x01:  /* RGB565 (No alpha) */
            tex_color = PVR_TXRFMT_RGB565;
            break;

        case 0x02:  /* ARGB4444 (4-bit alpha, 4-bit RGB) */
            tex_color = PVR_TXRFMT_ARGB4444;
            break;

        case 0x03:  /* YUV422 */
            tex_color = PVR_TXRFMT_YUV422;
            break;

        case 0x04:  /* Bump-mapping data */
            tex_color = PVR_TXRFMT_BUMP;
            break;

        case 0x05:  /* 4-bit palleted texture */
            tex_color = PVR_TXRFMT_PAL4BPP;
            break;

        case 0x06:  /* 8-bit palleted texture */
            tex_color = PVR_TXRFMT_PAL8BPP;
            break;

        default:    /* Non-translucent RGB565 */
            tex_color = PVR_TXRFMT_RGB565;
            break;
    }

    /* Determine texture data format */
    uint32_t tex_format;
    switch (header->data_format) {
        case 0x01:  /* Square twiddled */
            tex_format = PVR_TXRFMT_TWIDDLED;
            break;

        case 0x03:  /* VQ twiddled */
            tex_format = PVR_TXRFMT_VQ_ENABLE;
            break;

        case 0x09:  /* Rectangle */
            tex_format = PVR_TXRFMT_NONTWIDDLED;
            break;

        case 0x0B:  /* Rectangular stride */
            tex_format = PVR_TXRFMT_X32_STRIDE | PVR_TXRFMT_NONTWIDDLED;
            break;

        case 0x0D:  /* Rectangular twiddled */
            tex_format = PVR_TXRFMT_TWIDDLED;
            break;

        case 0x10:  /* Small VQ */
            tex_format = PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED;
            break;

        default:
            tex_format = PVR_TXRFMT_NONE;
            break;
    }

    /* Determine data size and allocate VRAM */
    size_t tex_size = size - (((uint8_t *)header - buffer) + sizeof(pvr_header_t));
    pvr_ptr_t texture_vram = pvr_mem_malloc(tex_size);
    if(!texture_vram) {
        dbglog(DBG_ERROR, "Error allocating vram in %s!\n", __func__);
        return 0;
    }

    /* Load texture data into VRAM */
    pvr_txr_load((uint8_t *)header + sizeof(pvr_header_t), texture_vram, tex_size);

    /* Assign our texture and return with texture data size */
    *texture = (pvr_texture_t) {
        .buffer = texture_vram,
        .width = header->width,
        .height = header->height,
        .format = tex_format | tex_color,
    };

    return tex_size;
}

/* Load a .PVR file into VRAM and get a pvr_texture_t from it */
size_t texture_load(pvr_texture_t *texture, const char *filename) {
    /* Clear texture */
    *texture = (pvr_texture_t) {
        .buffer = NULL,
        .width = 0,
        .height = 0,
        .format = 0,
    };

    /* Open file */
    FILE *texfile = fopen(filename, "rb");
    if (!texfile) {
        dbglog(DBG_ERROR, "Cannot open texture %s!\n", filename);
        return 0;
    }

    /* Get filesize */
    fseek(texfile, 0, SEEK_END);
    size_t texfile_size = ftell(texfile);
    fseek(texfile, 0, SEEK_SET);

    /* Create temporary buffer for texture processing */
    uint8_t *texbuf = calloc(1, texfile_size);
    if (!texbuf) {
        dbglog(DBG_ERROR, "calloc error in %s!\n", __func__);
        fclose(texfile);
        return 0;
    }

    /* Copy our texture file into the buffer */
    fread(texbuf, texfile_size, 1, texfile);
    fclose(texfile);

    /* Get texture from buffer */
    pvr_texture_t temp;
    size_t texture_size = texture_fill_from_buffer(&temp, texbuf, texfile_size);
    if (!texture_size || !temp.buffer) {
        dbglog(DBG_ERROR, "Error filling texture from %s in %s!\n",
               filename, __func__);
        free(texbuf);
        return 0;
    }

    /* We're done with the buffer */
    free(texbuf);

    *texture = temp;

    return texture_size;
}

/* Invalidated textures should always have
   PVR pointers set to NULL */
bool texture_valid(pvr_texture_t *texture) {
    return texture->buffer != NULL;
}

/* Free texture memory and invalidate structure */
void texture_free(pvr_texture_t *texture) {
    if(texture->buffer) {
        pvr_mem_free(texture->buffer);
    }

    *texture = (pvr_texture_t) {
        .buffer = NULL,
        .width = 0,
        .height = 0,
        .format = 0,
    };
}

#ifdef ROMFONT
#define ROMFONT_SIZE (256 * 256 * 2)
size_t texture_load_romfont(pvr_texture_t *texture) {
    /* Start with a clear texture */
    *texture = (pvr_texture_t) {
        .buffer = NULL,
        .width = 0,
        .height = 0,
        .format = 0,
    };

    /* Allocate some texture memory for the font texture  */
    pvr_ptr_t font_tex = pvr_mem_malloc(ROMFONT_SIZE);
    if(!font_tex) {
        dbglog(DBG_ERROR, "Error allocating VRAM in %s\n", __func__);
        return 0;
    }

    /* Draw biosfont to the texture memory */
    uint16_t *vram = font_tex;
    for (size_t y = 0; y < 8; y++) {
        for (size_t x = 0; x < 16; x++) {
            bfont_draw(vram, 256, 0, y*16 + x);
            vram += 16;
        }
        vram += 23*256;
    }

    /* Set up the texture */
    *texture = (pvr_texture_t) {
        .buffer = font_tex,
        .width = 256,
        .height = 256,
        .format = PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED,
    };

    return ROMFONT_SIZE;
}
#endif // ROMFONT
