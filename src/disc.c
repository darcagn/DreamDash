/*
   disc.c
   Copyright (C)2024 darcagn

   Derived from DreamShell GDPlay module
   Copyright (C)2014 megavolt85
   Copyright (C)2024 SWAT
*/

#include <kos.h>
#include <stdlib.h>
#include <zlib/zlib.h>
#include "disc.h"
#include "log.h"
#include "utility.h"

#define RUNGZ_SIZE 65280

ip_meta_t *ip_info;
static void *bios_patch;
static int cmd_response;
static int status;
static int disc_type;

kthread_t *check_gdrom_thd;
int kill_gdrom_thd = 0;

extern void gdplay_run_game(void *bios_patch);

static void set_info() {
    int lba = 45150;
    char pbuff[2048];

    cdrom_reinit();
    cdrom_get_status(&status, &disc_type);

    if(disc_type == CD_CDROM_XA) {
        CDROM_TOC toc;

        if(cdrom_read_toc(&toc, 0) != ERR_OK) {
            printf("Error reading disc TOC!\n");
            return;
        }

        if(!(lba = cdrom_locate_data_track(&toc))) {
            printf("Error locating data track on disc!\n");
            return;
        }
    }

    cmd_response = cdrom_read_sectors(pbuff, lba, 1);

    if(cmd_response != ERR_OK) {
        if(cmd_response == (ERR_DISC_CHG || ERR_NO_DISC)) {
            return;
        } else {
            printf("Error %d reading disc at LBA %d\n", cmd_response, lba);
            return;
        }
    }

    ip_info = (ip_meta_t *) pbuff;

    if(strncmp(ip_info->hardware_ID, "SEGA", 4)) {
        printf("No valid initial program (IP.BIN) found.\n");
        return;
     }

    printf("\nDisc header info:\n");
    printf("   Hardware ID:\t%.*s\n", 16, ip_info->hardware_ID);
    printf("   Maker ID:\t%.*s\n", 16, ip_info->maker_ID);
    printf("   Header CRC:\t%.*s\n", 5, ip_info->ks);
    printf("   Disc Number:\t%c of %c\n", ip_info->disk_num[0], ip_info->disk_num[2]);
    printf("   Region(s):\t%.*s\n", 3, ip_info->country_codes);
    printf("   Control:\t%.*s\n", 4, ip_info->ctrl);
    printf("   Devices:\t%.*s\n", 1, ip_info->dev);
    printf("   VGA support:\t%s\n", ip_info->VGA[0] == '1'? "Yes":"No");
    printf("   Windows CE:\t%s\n", ip_info->WinCE[0] == '1'? "Yes":"No");
    printf("   Product ID:\t%.*s\n", 10, ip_info->product_ID);
    printf("   Version:\t%.*s\n", 6, ip_info->product_version);
    printf("   Date:\t%c%c%c%c-%c%c-%c%c\n", ip_info->release_date[0],
                                             ip_info->release_date[1],
                                             ip_info->release_date[2],
                                             ip_info->release_date[3],
                                             ip_info->release_date[4],
                                             ip_info->release_date[5],
                                             ip_info->release_date[6],
                                             ip_info->release_date[7]);
    printf("   Boot file:\t%.*s\n", 16, ip_info->boot_file);
    printf("   Developer:\t%.*s\n", 16, ip_info->software_maker_info);
    printf("   Title:\t%.*s\n", 128, ip_info->title);

    fflush(stdout);
}

static void *check_gdrom() {
    cdrom_init();

    while(!kill_gdrom_thd) {
        if(cdrom_get_status(&status, &disc_type) == ERR_OK) {
            switch(status) {
                case CD_STATUS_OPEN:
                case CD_STATUS_NO_DISC:
                if(ip_info) {
                    printf("\nPlease insert disc and close drive lid...\n");
                    ip_info = NULL;
                }
                break;
            default:
                switch(disc_type) {
                    case CD_CDROM_XA:
                    case CD_GDROM:
                        if(!ip_info)
                            set_info();
                        break;
                }
            }
        }
        thd_pass();
    }

    return NULL;
}

void disc_launch(void) {
    printf("Shutting down KOS and lauching disc... have fun!\n\n");
    fflush(stdout);
    kill_gdrom_thd = 1;
    thd_join(check_gdrom_thd, NULL);

    gdplay_run_game(bios_patch);
}

void disc_shutdown(void) {
    kill_gdrom_thd = 1;
    thd_join(check_gdrom_thd, NULL);

    if(bios_patch)
        free(bios_patch);
}

int disc_init(void) {
    // TODO: Check if GD-ROM drive is available is available
    // If not, dash_log(DBG_INFO, "No GD-ROM drive found."); return -1;

    bios_patch = decompress_file_aligned("/rd/rungd.bin.gz", 32, RUNGZ_SIZE);

    check_gdrom_thd = thd_create(1, check_gdrom, NULL);

    dash_log(DBG_INFO, "GD-ROM initialized.");

    return 0;
}
