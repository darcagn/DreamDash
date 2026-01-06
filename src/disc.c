/*
   disc.c
   Copyright (C)2024-2026 darcagn

   Derived from DreamShell GDPlay module
   Copyright (C)2014 megavolt85
   Copyright (C)2024 SWAT
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kos/dbglog.h>
#include <kos/mutex.h>
#include <kos/thread.h>

#include <dc/cdrom.h>
#include <dc/pvr.h>

#include <zlib/zlib.h>

#include "disc.h"
#include "utility.h"
#include "drawing.h"
#include "dreamdash.h"
#include "input.h"
#include "scene.h"
#include "texture.h"

#define RUNGZ_FILE "/rd/rungd.bin.gz"
#define RUNGZ_SIZE 65280

static mutex_t ip_info_mutex = MUTEX_INITIALIZER;
static ip_meta_t *ip_info;
static char pbuff[2048];

static pvr_texture_t gdtex;

static int cmd_response;
static int status;
static int disc_type;

kthread_t *check_gdrom_thd;
int kill_gdrom_thd = 0;

/* mutex must be locked when this is called */
static void set_info_locked() {
    int lba = 45150;

    cdrom_reinit();
    cdrom_get_status(&status, &disc_type);

    if(disc_type == CD_CDROM_XA) {
        cd_toc_t toc;

        if(cdrom_read_toc(&toc, 0) != ERR_OK) {
            dbglog(DBG_ERROR, "Error reading disc TOC!\n");
            return;
        }

        if(!(lba = cdrom_locate_data_track(&toc))) {
            dbglog(DBG_ERROR, "Error locating data track on disc!\n");
            return;
        }
    }

    cmd_response = cdrom_read_sectors(pbuff, lba, 1);

    if(cmd_response != ERR_OK) {
        if(cmd_response == ERR_DISC_CHG || cmd_response == ERR_NO_DISC) {
            return;
        } else {
            dbglog(DBG_ERROR, "Error %d reading disc at LBA %d\n", cmd_response, lba);
            return;
        }
    }

    ip_info = (ip_meta_t *) pbuff;

    if(strncmp(ip_info->hardware_ID, "SEGA", 4)) {
        dbglog(DBG_ERROR, "No valid initial program (IP.BIN) found.\n");
        ip_info = NULL;
        return;
     }

        fflush(stdout);
}

/* Clear disc state when disc is removed */
static void disc_clear_status(void) {
    mutex_lock(&ip_info_mutex);
    if(ip_info) {
        dbglog(DBG_INFO, "\nPlease insert disc and close drive lid...\n");
        ip_info = NULL;
    }
    mutex_unlock(&ip_info_mutex);

    texture_free(&gdtex);
}

/* Fill disc state when disc is inserted */
static void disc_fill_status(void) {
    mutex_lock(&ip_info_mutex);
    if(!ip_info) {
        set_info_locked();

        if(ip_info) {
            dbglog(DBG_INFO, "Playable disc inserted: %.*s\n", 128, ip_info->title);
        }
    }

    mutex_unlock(&ip_info_mutex);


    if(!texture_valid(&gdtex)) {
        if(file_exists("/cd/0GDTEX.PVR")) {
            texture_load(&gdtex, "/cd/0GDTEX.PVR");
        }
    }
}

static void *check_gdrom(void *unused) {
    (void)unused;

    cdrom_init();

    while(!kill_gdrom_thd) {
        if(cdrom_get_status(&status, &disc_type) == ERR_OK) {
            switch(status) {
                case CD_STATUS_OPEN:
                case CD_STATUS_NO_DISC:
                    disc_clear_status();
                    break;
                default:
                    switch(disc_type) {
                        case CD_CDROM_XA:
                        case CD_GDROM:
                            disc_fill_status();
                            break;
                    }
                    break;
            }
        }
        thd_pass();
    }

    return NULL;
}

void disc_launch(void) {
    dbglog(DBG_INFO, "Shutting down KOS and lauching disc... have fun!\n\n");

    /* Open syscalls patch */
    gzFile rungz = gzopen(RUNGZ_FILE, "rb");
    if(!rungz) {
        dbglog(DBG_ERROR, "Error opening %s!\n", RUNGZ_FILE);
        return;
    }

    /* Decompress patched syscalls into place */
    if(gzread(rungz, (void *)0x8C000100, RUNGZ_SIZE) != RUNGZ_SIZE) {
        dbglog(DBG_ERROR, "Error decompressing %s\n!", RUNGZ_FILE);
        gzclose(rungz);
        return;
    }

    /* Clean up */
    gzclose(rungz);
    disc_shutdown();
    fflush(stdout);

    /* Disable and invalidate the cache */
    *(volatile unsigned long *)0xFF00001C = 0x0808;

    /* Bye bye! */
    ((void (*)(volatile unsigned short))0x8C000120)(0xFFF);
    __builtin_unreachable();
}



bool disc_ready(void) {
    mutex_lock(&ip_info_mutex);
    bool ready = ip_info != NULL;
    mutex_unlock(&ip_info_mutex);
    return ready;
}

void disc_shutdown(void) {
    kill_gdrom_thd = 1;
    thd_join(check_gdrom_thd, NULL);

    texture_free(&gdtex);
}

void disc_init(void) {
    // TODO: Check if GD-ROM drive is available is available

    if(!file_exists("/rd/rungd.bin.gz")) {
        dbglog(DBG_ERROR, "Error accessing rungd.bin.gz!\n");
    }

    check_gdrom_thd = thd_create(1, check_gdrom, NULL);
    if (!check_gdrom_thd) {
        dbglog(DBG_ERROR, "Failed to create GD-ROM thread!\n");
        return;
    }

    dbglog(DBG_INFO, "GD-ROM initialized.\n");
}

void draw_gdtex(size_t x, size_t y) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;

    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, gdtex.format,
                     gdtex.width, gdtex.height,
                     gdtex.buffer, PVR_FILTER_NONE);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;

    vert.x = x;
    vert.y = y;
    vert.z = 100;
    vert.u = 0.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x + gdtex.width;
    vert.y = y;
    vert.z = 100;
    vert.u = 1.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x;
    vert.y = y + gdtex.height;
    vert.z = 100;
    vert.u = 0.0;
    vert.v = 1.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x + gdtex.width;
    vert.y = y + gdtex.height;
    vert.z = 100;
    vert.u = 1.0;
    vert.v = 1.0;
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));
}

size_t line_height = DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING;

void disc_draw(void) {
    char disc_string[256];

    rect_t menuRect = (rect_t) {32, 32, DRAW_SCREEN_WIDTH - 64, DRAW_SCREEN_HEIGHT - 64};

    if (!ip_info) {
        rect_t info_rect = (rect_t) {
            menuRect.left + 140,
            menuRect.top + (DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING + 2) * 5,
            menuRect.width - 280,
            (DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING + 2) * 2
        };

        draw_rect_outline(info_rect, 100, COL_ALPHA(COL_BLACK, 128), COL_ALPHA(COL_BLACK, 96), 4);

        sprintf(disc_string, "    Please insert a");
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, 1);
        sprintf(disc_string, "  valid Dreamcast disc.");
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, 2);
    } else {

        rect_t info_rect = (rect_t) {
            menuRect.left + 280,
            menuRect.top,
            menuRect.width - 280,
            (DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING + 2) * 11
        };

        int i = 1;

        draw_box_outline(info_rect.left, info_rect.top, info_rect.width, info_rect.height,
                        100, COL_ALPHA(COL_BLACK, 128), COL_ALPHA(COL_BLACK, 96), 4);

        sprintf(disc_string, "%.*s", 128, ip_info->title);
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "%.*s", 16, ip_info->software_maker_info);
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "Product ID: %.*s", 10, ip_info->product_ID);
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "Version: %.*s", 6, ip_info->product_version);
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "Date: %c%c%c%c-%c%c-%c%c", ip_info->release_date[0],
                                                        ip_info->release_date[1],
                                                        ip_info->release_date[2],
                                                        ip_info->release_date[3],
                                                        ip_info->release_date[4],
                                                        ip_info->release_date[5],
                                                        ip_info->release_date[6],
                                                        ip_info->release_date[7]);
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "Disc %c of %c", ip_info->disk_num[0], ip_info->disk_num[2]);
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "VGA support: %s", ip_info->VGA[0] == '1'? "Yes":"No");
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "Region(s): %.*s", 3, ip_info->country_codes);
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "Control: %.*s", 4, ip_info->ctrl);
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "Devices: %.*s", 1, ip_info->dev);
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        sprintf(disc_string, "Windows CE: %s", ip_info->WinCE[0] == '1'? "Yes":"No");
        draw_string_rect_line(info_rect, 103, COL_WHITE, disc_string, i++);

        rect_t start_rect = (rect_t) {
            menuRect.left + 160,
            menuRect.top + (DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING + 2) * 12,
            menuRect.width - 335,
            (DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING + 2) * 1
        };

        draw_rect_outline(start_rect, 100, COL_ALPHA(COL_BLACK, 128), COL_ALPHA(COL_BLACK, 96), 4);
        sprintf(disc_string, "Press Start to Play");
        draw_string_rect(start_rect, 103, COL_WHITE, disc_string);

        if(texture_valid(&gdtex)) {
            draw_gdtex(32, 64);
        }
    }
}

void disc_input(void) {
    uint32_t input = get_input();

    if (input & CONT_START) {
        if (disc_ready()) {
            disc_launch();
        }
    } else if (input & CONT_B) {
        mainmenu_enter();
    }
}

void disc_enter(void) {
    scene_id = SCENE_DISC;
    scene_input_fn = disc_input;
    scene_draw_fn = disc_draw;
}
