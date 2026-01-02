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
#include "utility.h"

#define RUNGZ_FILE "/rd/rungd.bin.gz"
#define RUNGZ_SIZE 65280

ip_meta_t *ip_info;
static int cmd_response;
static int status;
static int disc_type;

kthread_t *check_gdrom_thd;
int kill_gdrom_thd = 0;

static void set_info() {
    int lba = 45150;
    char pbuff[2048];

    cdrom_reinit();
    cdrom_get_status(&status, &disc_type);

    if(disc_type == CD_CDROM_XA) {
        cd_toc_t toc;

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
        if(cmd_response == ERR_DISC_CHG || cmd_response == ERR_NO_DISC) {
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

printf("\nDisc header info:\n"
       "\tHardware ID:\t%.*s\n"
       "\tMaker ID:\t%.*s\n"
       "\tHeader CRC:\t%.*s\n"
       "\tDisc Number:\t%c of %c\n"
       "\tRegion(s):\t%.*s\n"
       "\tControl:\t%.*s\n"
       "\tDevices:\t%.*s\n"
       "\tVGA support:\t%s\n"
       "\tWindows CE:\t%s\n"
       "\tProduct ID:\t%.*s\n"
       "\tVersion:\t%.*s\n"
       "\tDate:\t\t%c%c%c%c-%c%c-%c%c\n"
       "\tBoot file:\t%.*s\n"
       "\tDeveloper:\t%.*s\n"
       "\tTitle:\t\t%.*s\n",
       16, ip_info->hardware_ID,
       16, ip_info->maker_ID,
       5, ip_info->ks,
       ip_info->disk_num[0], ip_info->disk_num[2],
       3, ip_info->country_codes,
       4, ip_info->ctrl,
       1, ip_info->dev,
       ip_info->VGA[0] == '1' ? "Yes" : "No",
       ip_info->WinCE[0] == '1' ? "Yes" : "No",
       10, ip_info->product_ID,
       6, ip_info->product_version,
       ip_info->release_date[0], ip_info->release_date[1], ip_info->release_date[2], ip_info->release_date[3],
       ip_info->release_date[4], ip_info->release_date[5], ip_info->release_date[6], ip_info->release_date[7],
       16, ip_info->boot_file,
       16, ip_info->software_maker_info,
       128, ip_info->title);
    fflush(stdout);
}

static void *check_gdrom(void *unused) {
    (void)unused;

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

    /* Open syscalls patch */
    gzFile rungz = gzopen(RUNGZ_FILE, "rb");
    if(!rungz) {
        dbglog(DBG_ERROR, "Error opening %s!", RUNGZ_FILE);
        return;
    }

    /* Decompress patched syscalls into place */
    if(gzread(rungz, (void *)0x8C000100, RUNGZ_SIZE) != RUNGZ_SIZE) {
        dbglog(DBG_ERROR, "Error decompressing %s!", RUNGZ_FILE);
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

void disc_shutdown(void) {
    kill_gdrom_thd = 1;
    thd_join(check_gdrom_thd, NULL);
}

int disc_init(void) {
    // TODO: Check if GD-ROM drive is available is available
    // If not, dbglog(DBG_INFO, "No GD-ROM drive found."); return -1;

    if(!file_exists("/rd/rungd.bin.gz")) {
        dbglog(DBG_ERROR, "Error accessing rungd.bin.gz!");
    }

    check_gdrom_thd = thd_create(1, check_gdrom, NULL);

    dbglog(DBG_INFO, "GD-ROM initialized.");

    return 0;
}
