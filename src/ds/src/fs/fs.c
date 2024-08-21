/** 
 * \file      fs.c
 * \brief     Filesystem
 * \date      2013-2024
 * \author    SWAT
 * \copyright	http://www.dc-swat.ru
 */

#include <kos.h>
#include <string.h>
#include <stdlib.h>
#include <utility.h>
#include "fs.h"
#include "drivers/sd.h"
#include "drivers/g1_ide.h"
#include "utils.h"

typedef struct romdisk_hdr {
    char    magic[8];           /* Should be "-rom1fs-" */
    uint32  full_size;          /* Full size of the file system */
    uint32  checksum;           /* Checksum */
    char    volume_name[16];    /* Volume name (zero-terminated) */
} romdisk_hdr_t;

typedef struct blockdev_devdata {
    uint64_t block_count;
    uint64_t start_block;
} sd_devdata_t;

typedef struct ata_devdata {
    uint64_t block_count;
    uint64_t start_block;
    uint64_t end_block;
} ata_devdata_t;

#define MAX_PARTITIONS 4

static kos_blockdev_t sd_dev[MAX_PARTITIONS];
static kos_blockdev_t g1_dev[MAX_PARTITIONS];
static kos_blockdev_t g1_dev_dma[MAX_PARTITIONS];

static uint32 ntohl_32(const void *data) {
    const uint8 *d = (const uint8*)data;
    return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
}


static int check_partition(uint8 *buf, int partition) {
	int pval;
	
	if(buf[0x01FE] != 0x55 || buf[0x1FF] != 0xAA) {
//		dash_log(DBG_DEBUG, "Device doesn't appear to have a MBR\n");
		return -1;
	}
	
	pval = 16 * partition + 0x01BE;

	if(buf[pval + 4] == 0) {
//		dash_log(DBG_DEBUG, "Partition empty: 0x%02x\n", buf[pval + 4]);
		return -1;
	}
	
	return 0;
}


int InitSDCard() {

	dash_log(DBG_INFO, "Checking for SD card...\n");
	uint8 partition_type;
	int part = 0, fat_part = 0;
	char path[8];
	uint8 buf[512];
	kos_blockdev_t *dev;

	if(sdc_init()) {
		scif_init();
		dash_log(DBG_INFO, "\nSD card not found.\n");
		return -1;
	}

	dash_log(DBG_INFO, "SD card initialized, capacity %" PRIu32 " MB\n",
		(uint32)(sdc_get_size() / 1024 / 1024));

//	if(sdc_print_ident()) {
//		dash_log(DBG_INFO, "SD card read CID error\n");
//		return -1;
//	}

	if(sdc_read_blocks(0, 1, buf)) {
		dash_log(DBG_ERROR, "Can't read MBR from SD card\n");
		return -1;
	}

	memset(&sd_dev[0], 0, sizeof(sd_dev));

	for(part = 0; part < MAX_PARTITIONS; part++) {

		dev = &sd_dev[part];

		if(check_partition(buf, part)) {
			continue;
		}
		if(sdc_blockdev_for_partition(part, dev, &partition_type)) {
			continue;
		}

		if(!part) {
			strcpy(path, "/sd");
			path[3] = '\0';
		}
		else {
			sprintf(path, "sd%d", part);
		}

		/* Check to see if the MBR says that we have a Linux partition. */
		if(is_ext2_partition(partition_type)) {

			dash_log(DBG_INFO, "Detected EXT2 filesystem on partition %d\n", part);

			if(fs_ext2_init()) {

				dash_log(DBG_INFO, "Could not initialize fs_ext2!\n");
				dev->shutdown(dev);
			}
			else {
				dash_log(DBG_INFO, "Mounting filesystem...\n");

				if(fs_ext2_mount(path, dev, FS_EXT2_MOUNT_READWRITE)) {
					dash_log(DBG_INFO, "Could not mount device as ext2fs.\n");
					dev->shutdown(dev);
				}
			}

		}
		else if((fat_part = is_fat_partition(partition_type))) {

			dash_log(DBG_INFO, "Detected FAT%d filesystem on partition %d\n", fat_part, part);

			if(fs_fat_init()) {

				dash_log(DBG_INFO, "Could not initialize fs_fat!\n");
				dev->shutdown(dev);
			}
			else {
				/* Need full disk block device for FAT */
				dev->shutdown(dev);
				if(sdc_blockdev_for_device(dev)) {
					continue;
				}

				dash_log(DBG_INFO, "Mounting filesystem...\n");

				if(fs_fat_mount(path, dev, NULL, part)) {
					dash_log(DBG_INFO, "Could not mount device as fatfs.\n");
					dev->shutdown(dev);
				}
			}
		}
		else {
			dash_log(DBG_INFO, "Unknown filesystem: 0x%02x\n", partition_type);
			dev->shutdown(dev);
		}
	}
	return 0;
}


int InitIDE() {

	dash_log(DBG_INFO, "Checking for G1 ATA devices...\n");
	uint8 partition_type;
	int part = 0, fat_part = 0;
	char path[8];
	uint8 buf[512];
	kos_blockdev_t *dev;
	kos_blockdev_t *dev_dma;

	if(g1_ata_init()) {
		return -1;
	}

	/* Read the MBR from the disk */
	if(g1_ata_lba_mode()) {
		if(g1_ata_read_lba(0, 1, (uint16_t *)buf) < 0) {
			dash_log(DBG_ERROR, "Can't read MBR from IDE by LBA\n");
			return -1;
		}
	}
	else {
		if(g1_ata_read_chs(0, 0, 1, 1, (uint16_t *)buf) < 0) {
			dash_log(DBG_ERROR, "Can't read MBR from IDE by CHS\n");
			return -1;
		}
	}

	memset(&g1_dev[0], 0, sizeof(g1_dev));
	memset(&g1_dev_dma[0], 0, sizeof(g1_dev_dma));

	for(part = 0; part < MAX_PARTITIONS; part++) {

		dev = &g1_dev[part];
		dev_dma = &g1_dev_dma[part];

		if(check_partition(buf, part)) {
			continue;
		}
		if(g1_ata_blockdev_for_partition(part, 1, dev, &partition_type)) {
			continue;
		}

		if(!part) {
			strcpy(path, "/ide");
			path[4] = '\0';
		}
		else {
			sprintf(path, "/ide%d", part);
			path[strlen(path)] = '\0';
		}

		/* Check to see if the MBR says that we have a EXT2 or FAT partition. */
		if (is_ext2_partition(partition_type)) {

			dash_log(DBG_INFO, "Detected EXT2 filesystem on partition %d\n", part);

			if (fs_ext2_init()) {
				dash_log(DBG_INFO, "Could not initialize fs_ext2!\n");
				dev->shutdown(dev);
			}
			else {

				dash_log(DBG_INFO, "Mounting filesystem...\n");

				if (fs_ext2_mount(path, dev, FS_EXT2_MOUNT_READWRITE)) {
					dash_log(DBG_INFO, "Could not mount device as ext2fs.\n");
					dev->shutdown(dev);
				}
			}
		}
		else if ((fat_part = is_fat_partition(partition_type))) {

			dash_log(DBG_INFO, "Detected FAT%d filesystem on partition %d\n", fat_part, part);

			if (fs_fat_init()) {

				dash_log(DBG_INFO, "Could not initialize fs_fat!\n");
				dev->shutdown(dev);

			}
			else {
				/* Need full disk block device for FAT */
				dev->shutdown(dev);

				if (g1_ata_blockdev_for_device(0, dev)) {
					continue;
				}

				if (g1_ata_blockdev_for_device(1, dev_dma)) {
					dev_dma = NULL;
				}

				dash_log(DBG_INFO, "Mounting filesystem...\n");

				if(fs_fat_mount(path, dev, dev_dma, part)) {
					dash_log(DBG_INFO, "Could not mount device as fatfs.\n");
					dev->shutdown(dev);
					if (dev_dma) {
						dev_dma->shutdown(dev_dma);
					}
				}
			}
		}
		else {
			dash_log(DBG_INFO, "Unknown filesystem: 0x%02x\n", partition_type);
			dev->shutdown(dev);
		}
	}
	return 0;
}


int InitRomdisk() {
	
	int cnt = -1;
	uint32 size, addr;
	char path[32];
	uint8 *tmpb = (uint8 *)0x00100000;
	
	dash_log(DBG_INFO, "Checking for romdisk in the bios...\n");
	
	for(addr = 0x00100000; addr < 0x00200000; addr++) {

		if(tmpb[0] == 0x2d && tmpb[1] == 0x72 && tmpb[2] == 0x6f) {
			
			romdisk_hdr_t *romfs = (romdisk_hdr_t *)tmpb;

			if(strncmp(romfs->magic, "-rom1fs-", 8) || strncmp(romfs->volume_name, getenv("HOST"), 10)) {
				continue;
			}

			size = ntohl_32((const void *)&romfs->full_size);

			if(!size || size > 0x1F8000) {
				continue;
			}

			dash_log(DBG_INFO, "Detected romdisk at 0x%08lx, mounting...\n", (uint32)tmpb);

			if(cnt) {
				snprintf(path, sizeof(path), "/brd%d", cnt+1);
			} else {
				strncpy(path, "/brd", sizeof(path));
			}

			if(fs_romdisk_mount(path, (const uint8 *)tmpb, 0) < 0) {
				dash_log(DBG_INFO, "Error mounting romdisk at 0x%08lx\n", (uint32)tmpb);
			} else {
				dash_log(DBG_INFO, "Romdisk mounted as %s\n", path);
			}

			cnt++;
			tmpb += sizeof(romdisk_hdr_t) + size;
		}

		tmpb++;
	}

	return (cnt > -1 ? 0 : -1);
}


int RootDeviceIsSupported(const char *name) {
	if(!strncmp(name, "sd", 2) ||
		!strncmp(name, "ide", 3) ||
		!strncmp(name, "cd", 2) ||
		!strncmp(name, "pc", 2) ||
		!strncmp(name, "brd", 3)) {
		return 1;
	}
	return 0;
}

static int SearchRootCheck(char *device, char *path, char *file) {

	char check[NAME_MAX];

	if(file == NULL) {
		sprintf(check, "/%s%s", device, path);
	} else {
		sprintf(check, "/%s%s/%s", device, path, file);
	}

	if((file == NULL && DirExists(check)) || (file != NULL && FileExists(check))) {
		sprintf(check, "/%s%s", device, path);
		setenv("PATH", check, 1);
		return 1;
	}

	return 0;
}


int SearchRoot() {

	dirent_t *ent;
	file_t hnd;
	int detected = 0;

	hnd = fs_open("/", O_RDONLY | O_DIR);

	if(hnd < 0) {
		dash_log(DBG_ERROR, "Can't open root directory!\n");
		return -1;
	}

	while ((ent = fs_readdir(hnd)) != NULL) {

		if(!RootDeviceIsSupported(ent->name)) {
			continue;
		}

		dash_log(DBG_INFO, "Checking for root directory on /%s\n", ent->name);

		if(SearchRootCheck(ent->name, "/DS", "/lua/startup.lua") ||
			SearchRootCheck(ent->name, "", "/lua/startup.lua")) {
			detected = 1;
			break;
		}
	}

	fs_close(hnd);

	if (!detected) {
		dash_log(DBG_ERROR, "Can't find root directory.\n");
		setenv("PATH", "/ram", 1);
		setenv("TEMP", "/ram", 1);
		return -1;
	}

	if(strncmp(getenv("PATH"), "/pc", 3) && DirExists("/pc")) {

		dash_log(DBG_INFO, "Checking for root directory on /pc\n");

		if(!SearchRootCheck("pc", "", "/lua/startup.lua")) {
			SearchRootCheck("pc", "/DS", "/lua/startup.lua");
		}
	}

	if(	!strncmp(getenv("PATH"), "/sd", 3) || 
		!strncmp(getenv("PATH"), "/ide", 4) || 
		!strncmp(getenv("PATH"), "/pc", 3)) {
		setenv("TEMP", getenv("PATH"), 1);
	} else {
		setenv("TEMP", "/ram", 1);
	}

	dash_log(DBG_INFO, "Root directory is %s\n", getenv("PATH"));
//	dash_log(DBG_INFO, "Temp directory is %s\n", getenv("TEMP"));
	return 0;
}
