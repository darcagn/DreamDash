#ifndef DREAMDASH_TEXTURE_H
#define DREAMDASH_TEXTURE_H

#include <stdint.h>

#include <dc/pvr.h>

typedef struct pvr_texture {
    pvr_ptr_t buffer;
    size_t width;
    size_t height;
    uint32_t format;
} pvr_texture_t;

bool texture_valid(pvr_texture_t *texture);
size_t texture_load(pvr_texture_t *texture, const char *filename);
size_t texture_load_romfont(pvr_texture_t *texture);
void texture_free(pvr_texture_t *texture);

#endif //DREAMDASH_TEXTURE_H
