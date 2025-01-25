/*
 * FatFs for the Sega Dreamcast
 *
 * This file is part of the FatFs module, a generic FAT filesystem
 * module for small embedded systems. This version has been ported and
 * optimized specifically for the Sega Dreamcast platform.
 *
 * Copyright (c) 2007-2025 Ruslan Rostovtsev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <dc/g1ata.h>
#include <dc/sd.h>
#include <dc/scif.h>

#include "fatfs.h"
#include "../log.h"

#define MAX_PARTITIONS 4

static kos_blockdev_t *sd_dev = NULL;
static kos_blockdev_t *g1_dev = NULL;
static kos_blockdev_t *g1_dev_dma = NULL;

static int is_fat_partition(uint8_t partition_type) {
    switch(partition_type) {
        case 0x04: /* 32MB */
        case 0x06: /* Over 32 to 2GB */
            return 16;
        case 0x0B:
        case 0x0C:
            return 32;
        default:
            return 0;
    }
}

static int check_partition(uint8_t *buf, int partition) {
    int pval;
    
    if (buf[0x01FE] != 0x55 || buf[0x1FF] != 0xAA) {
		// dash_log(DBG_DEBUG, "Device doesn't appear to have a MBR\n");
        return -1;
    }
    
    pval = 16 * partition + 0x01BE;

    if (buf[pval + 4] == 0) {
		// dash_log(DBG_DEBUG, "Partition empty: 0x%02x\n", buf[pval + 4]);
        return -1;
    }
    
    return 0;
}

typedef struct sd_devdata {
    uint64_t block_count;
    uint64_t start_block;
} sd_devdata_t;

static int sd_blockdev_for_device(kos_blockdev_t *rv) {
    sd_devdata_t *ddata;

    // if (!initted) {
    //     errno = ENXIO;
    //     return -1;
    // }

    if (!rv) {
        errno = EFAULT;
        return -1;
    }

    /* Allocate the device data */
    if (!(ddata = (sd_devdata_t *)malloc(sizeof(sd_devdata_t)))) {
        errno = ENOMEM;
        return -1;
    }

    ddata->start_block = 0;
    ddata->block_count = (sd_get_size() / 512);
    rv->dev_data = ddata;

    return 0;
}

int fs_fat_mount_sd() {

    uint8_t partition_type;
    int part = 0, fat_part = 0;
    char path[8];
    uint8_t buf[512];
    kos_blockdev_t *dev;

    dash_log(DBG_INFO, "Checking for SD card...\n");

    if (sd_init()) {
        scif_init();
        dash_log(DBG_INFO, "\nSD card not found.\n");
        return -1;
    }

    dash_log(DBG_INFO, "SD card initialized, capacity %" PRIu32 " MB\n",
        (uint32)(sd_get_size() / 1024 / 1024));

    if (sd_read_blocks(0, 1, buf)) {
        dash_log(DBG_ERROR, "Can't read MBR from SD card\n");
        return -1;
    }

    if (!sd_dev) {
        sd_dev = malloc(sizeof(kos_blockdev_t) * MAX_PARTITIONS);
    }
    if (!sd_dev) {
        dash_log(DBG_ERROR, "Can't allocate memory for SD card partitions\n");
        return -1;
    }

    memset(&sd_dev[0], 0, sizeof(kos_blockdev_t) * MAX_PARTITIONS);

    for (part = 0; part < MAX_PARTITIONS; part++) {

        dev = &sd_dev[part];

        if (check_partition(buf, part)) {
            continue;
        }
        if (sd_blockdev_for_partition(part, dev, &partition_type)) {
            continue;
        }

        if (!part) {
            strcpy(path, "/sd");
            path[3] = '\0';
        }
        else {
            sprintf(path, "sd%d", part);
        }

        /* Check to see if the MBR says that we have a FAT partition. */
        fat_part = is_fat_partition(partition_type);

        if (fat_part) {

            dash_log(DBG_INFO, "Detected FAT%d filesystem on partition %d\n", fat_part, part);

            if (fs_fat_init()) {
                dash_log(DBG_INFO, "Could not initialize fs_fat!\n");
                dev->shutdown(dev);
            }
            else {
                /* Need full disk block device for FAT */
                dev->shutdown(dev);
                if (sd_blockdev_for_device(dev)) {
                    continue;
                }

                dash_log(DBG_INFO, "Mounting filesystem...\n");

                if (fs_fat_mount(path, dev, NULL, part)) {
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

int fs_fat_mount_ide() {

    uint8_t partition_type;
    int part = 0, fat_part = 0;
    char path[8];
    uint8_t buf[512];
    kos_blockdev_t *dev;
    kos_blockdev_t *dev_dma;

    dash_log(DBG_INFO, "Checking for G1 ATA devices...\n");

    if (g1_ata_init()) {
        return -1;
    }

    /* Read the MBR from the disk */
    if (g1_ata_lba_mode()) {
        if (g1_ata_read_lba(0, 1, (uint16_t *)buf) < 0) {
            dash_log(DBG_ERROR, "Can't read MBR from IDE by LBA\n");
            return -1;
        }
    }
    else {
        if (g1_ata_read_chs(0, 0, 1, 1, (uint16_t *)buf) < 0) {
            dash_log(DBG_ERROR, "Can't read MBR from IDE by CHS\n");
            return -1;
        }
    }

    if (g1_dev == NULL) {
        g1_dev = malloc(sizeof(kos_blockdev_t) * MAX_PARTITIONS);
        g1_dev_dma = malloc(sizeof(kos_blockdev_t) * MAX_PARTITIONS);
    }
    if (!g1_dev || !g1_dev_dma) {
        dash_log(DBG_ERROR, "Can't allocate memory for IDE partitions\n");
        return -1;
    }

    memset(&g1_dev[0], 0, sizeof(kos_blockdev_t) * MAX_PARTITIONS);
    memset(&g1_dev_dma[0], 0, sizeof(kos_blockdev_t) * MAX_PARTITIONS);

    for (part = 0; part < MAX_PARTITIONS; part++) {

        dev = &g1_dev[part];
        dev_dma = &g1_dev_dma[part];

        if (check_partition(buf, part)) {
            continue;
        }
        if (g1_ata_blockdev_for_partition(part, 0, dev, &partition_type)) {
            continue;
        }

        if (!part) {
            strcpy(path, "/ide");
            path[4] = '\0';
        }
        else {
            sprintf(path, "/ide%d", part);
            path[strlen(path)] = '\0';
        }

        /* Check to see if the MBR says that we have a FAT partition. */
        fat_part = is_fat_partition(partition_type);

        if (fat_part) {

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

                if (fs_fat_mount(path, dev, dev_dma, part)) {
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
