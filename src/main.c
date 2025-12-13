#include <kos.h>

#include "disc.h"
#include "drawing.h"
#include "input.h"
#include "log.h"
#include "menu.h"
#include "utility.h"

#include <dc/g1ata.h>
#include <dc/sd.h>

#ifdef FAT_LIBRARY_FATFS
#include "fatfs/fatfs.h"
#endif

#ifdef FAT_LIBRARY_KOSFAT
#include <fat/fs_fat.h>

static kos_blockdev_t sd_dev;
static kos_blockdev_t ide_dev;

void sdcard_init(void) {
    uint8_t partition_type;

    if (sd_init())
        return;

    if (sd_blockdev_for_partition(0, &sd_dev, &partition_type))
        return;

    if (fs_fat_mount("/sd", &sd_dev, FS_FAT_MOUNT_READWRITE))
        return;

    printf("mounted sd card at /sd\n");
}

void ide_init(void) {
    uint8_t partition_type;

    if (g1_ata_init())
        return;

    if (g1_ata_blockdev_for_partition(0, 1, &ide_dev, &partition_type))
        return;

    if (fs_fat_mount("/ide", &ide_dev, FS_FAT_MOUNT_READWRITE))
        return;

    printf("mounted ide partition at /ide\n");
}
#endif

int main(int argc, char **argv) {
    dash_log(DBG_INFO, "%s", kos_get_banner());

    uint32_t keys = get_input();
    if (keys & CONT_A && keys & CONT_B) {
        launch_dcload_serial();
    } else if (keys & CONT_X && keys & CONT_Y) {
        launch_dcload_ip();
    }

#ifdef FAT_LIBRARY_KOSFAT
    fs_fat_init();
    ide_init();
    sdcard_init();
#endif

#ifdef FAT_LIBRARY_FATFS
    fs_fat_mount_ide();
    fs_fat_mount_sd();
#endif

    disc_init();

    draw_init();
    back_init();

#ifdef AUTOBOOT
    if (keys & CONT_START) {
        menu_run();
    } else {
        try_boot();
        menu_run();
    }
#else
    menu_run();
#endif

    draw_exit();

    return 0;
}
