#include <kos/dbglog.h>

#include <dc/g1ata.h>
#include <dc/sd.h>

#ifdef FAT_LIBRARY_FATFS
#include <fatfs.h>
#endif

#ifdef FAT_LIBRARY_KOSFAT
#include <fat/fs_fat.h>
#endif

#ifdef FAT_LIBRARY_KOSFAT
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

    dbglog(DBG_INFO, "mounted sd card at /sd\n");
}

void ide_init(void) {
    uint8_t partition_type;

    if (g1_ata_init())
        return;

    if (g1_ata_blockdev_for_partition(0, 1, &ide_dev, &partition_type))
        return;

    if (fs_fat_mount("/ide", &ide_dev, FS_FAT_MOUNT_READWRITE))
        return;

    dbglog(DBG_INFO, "mounted ide partition at /ide\n");
}
#endif

void storage_init(void) {
#ifdef FAT_LIBRARY_KOSFAT
    fs_fat_init();
    ide_init();
    sdcard_init();
#endif

#ifdef FAT_LIBRARY_FATFS
    fs_fat_mount_ide();
    fs_fat_mount_sd();
#endif
}

void storage_exit(void) {
#ifdef FAT_LIBRARY_KOSFAT
    fs_fat_unmount("/ide");
    g1_ata_shutdown();
    fs_fat_unmount("/sd");
    sd_shutdown();
    fs_fat_shutdown();
#endif

#ifdef FAT_LIBRARY_FATFS
    fs_fat_unmount_ide();
    fs_fat_unmount_sd();
#endif
}
