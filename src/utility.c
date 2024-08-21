#include <kos.h>
#include <stdlib.h>
#include <string.h>
#include <zlib/zlib.h>
#include "drawing.h"
#include "fs.h"
#include "uthash/utlist.h"
#include "utility.h"

KOS_INIT_FLAGS(INIT_DEFAULT);

void dash_log(int level, const char *fmt, ...);

#define dbglog(lv, fmt, ...) dash_log(lv, fmt, ##__VA_ARGS__)

int list_cmp(ListItem *a, ListItem *b) {

    if (a->type == TYPE_DIR && b->type != TYPE_DIR) {
        return -1;
    } else if (a->type != TYPE_DIR && b->type == TYPE_DIR) {
        return 1;
    }

    return strcasecmp(a->name, b->name);
}

void free_dir(List *list) {

    ListItem *elt, *tmp;
    DL_FOREACH_SAFE(list->head, elt, tmp) {
        DL_DELETE(list->head, elt);
        free(elt);
    }
}

ListItem *get_item(List *list, int index) {

    ListItem *file = list->head;
    if (index == 0) {
        return file;
    }

    for (int i = 1; i < list->size; i++) {
        file = (ListItem *) file->next;
        if (index == i) {
            return file;
        }
    }

    return NULL;
}

void try_boot() {

    // first check for boot config
    if (file_exists("/sd/boot.cfg")) {
        char *path = read_file("/sd/boot.cfg", NULL);
        if (path != NULL) {
            trim(path);
            if (file_exists(path)) {
                exec(path);
            }
        }
    } else if (file_exists("/ide/boot.cfg")) {
        char *path = read_file("/ide/boot.cfg", NULL);
        if (path != NULL) {
            trim(path);
            if (file_exists(path)) {
                exec(path);
            }
        }
    }

    // then retrodream.bin
    launch_retrodream();

    // finally check for DS_CORE.BIN
    launch_dreamshell();
}

void trim(char *str) {

    char *pos = NULL;

    while ((pos = strrchr(str, '\n')) != NULL) {
        *pos = '\0';
    }

    size_t len = strlen(str) - 1;
    for (int i = len; i; i--) {
        if (str[i] > ' ') {
            break;
        }
        str[i] = '\0';
    }
}

void get_dir(List *list, const char *path) {

    dirent_t *ent;
    file_t fd;
    ListItem *entry;

    memset(list, 0, sizeof(List));
    strncpy(list->path, path, MAX_PATH - 1);

    if ((fd = fs_open(path, O_RDONLY | O_DIR)) != FILEHND_INVALID) {
        while ((ent = fs_readdir(fd)) != NULL) {

            // skip "."
            if (ent->name[0] == '.') {
                continue;
            }

            if (strncmp(ent->name, "dev", 3) == 0 || strncmp(ent->name, "pty", 3) == 0
                || strncmp(ent->name, "ram", 3) == 0 || strncmp(ent->name, "pc", 2) == 0
                || strncmp(ent->name, "cd", 2) == 0) {
                continue;
            }

            entry = (ListItem *) malloc(sizeof(ListItem));
            memset(entry, 0, sizeof(ListItem));

            strncpy(entry->name, ent->name, MAX_PATH - 1);
            if (list->path[strlen(list->path) - 1] != '/') {
                snprintf(entry->path, MAX_PATH - 1, "%s/%s", list->path, ent->name);
            } else {
                snprintf(entry->path, MAX_PATH - 1, "%s%s", list->path, ent->name);
            }

            entry->type = ent->attr == O_DIR ? TYPE_DIR : TYPE_FILE;
            if (entry->type == TYPE_FILE) {
                if (strstr(entry->name, ".bin") != NULL || strstr(entry->name, ".BIN") != NULL ||
                    strstr(entry->name, ".elf") != NULL || strstr(entry->name, ".ELF") != NULL) {
                    entry->type = TYPE_BIN;
                }
            }

            DL_APPEND(list->head, entry);
            list->size++;
        }

        DL_SORT(list->head, list_cmp);
        fs_close(fd);
    }
}

int file_exists(const char *fn) {
    file_t f;

    f = fs_open(fn, O_RDONLY);

    if (f == FILEHND_INVALID) {
        return 0;
    }

    fs_close(f);
    return 1;
}

int dir_exists(const char *dir) {
    file_t f;

    f = fs_open(dir, O_DIR | O_RDONLY);

    if (f == FILEHND_INVALID) {
        return 0;
    }

    fs_close(f);
    return 1;
}

char *read_file(const char *file, int *size) {

    file_t fd;
    ssize_t fsize;
    char *buffer = NULL;

    fd = fs_open(file, O_RDONLY);
    if (fd == FILEHND_INVALID) {
        printf("read_file: can't open %s\n", file);
        if (size != NULL) {
            *size = 0;
        }
        return NULL;
    }

    fsize = fs_total(fd);
    buffer = (char *) malloc(fsize);
    memset(buffer, 0, fsize);

    if (fs_read(fd, buffer, fsize) != fsize) {
        fs_close(fd);
        free(buffer);
        printf("read_file: can't read %s\n", file);
        if (size != NULL) {
            *size = 0;
        }
        return NULL;
    }

    fs_close(fd);

    if (size != NULL) {
        *size = fsize;
    }

    return buffer;
}

void *decompress_file_aligned(const char *file, int alignment, int output_size) {
    gzFile gzfile = gzopen(file, "rb");

    if(!gzfile) {
        dash_log(DBG_ERROR, "Error opening %s!", file);
        return NULL;
    }

    void *buffer = memalign(alignment, output_size);
    if(!buffer) {
        dash_log(DBG_ERROR, "Error in memalign!");
        return NULL;
    }

    if(gzread(gzfile, buffer, output_size) != output_size) {
        dash_log(DBG_ERROR, "Error decompressing %s!", file);
        return NULL;
    }

    gzclose(gzfile);

    return buffer;
}

void *decompress_file(const char *file, int output_size) {
    return decompress_file_aligned(file, 8, output_size);
}

int flash_get_region() {

    int start, size;
    uint8_t region[6] = {0};
    region[2] = *(uint8_t *) 0x0021A002;

    /* Find the partition */
    if (flashrom_info(FLASHROM_PT_SYSTEM, &start, &size) < 0) {
        dash_log(DBG_ERROR, "%s: can't find partition %d\n", __func__, FLASHROM_PT_SYSTEM);
    } else {
        /* Read the first 5 characters of that partition */
        if (flashrom_read(start, region, 5) < 0) {
            dash_log(DBG_ERROR, "%s: can't read partition %d\n", __func__, FLASHROM_PT_SYSTEM);
        }
    }

    if (region[2] == 0x58 || region[2] == 0x30) {
        return FLASHROM_REGION_JAPAN;
    } else if (region[2] == 0x59 || region[2] == 0x31) {
        return FLASHROM_REGION_US;
    } else if (region[2] == 0x5A || region[2] == 0x32) {
        return FLASHROM_REGION_EUROPE;
    } else {
        dash_log(DBG_ERROR, "%s: Unknown region code %02x\n", __func__, region[2]);
        return FLASHROM_REGION_UNKNOWN;
    }
}

int is_hacked_bios() {
    return (*(uint16_t *) 0xa0000000) == 0xe6ff;
}

int is_custom_bios() {
    return (*(uint16_t *) 0xa0000004) == 0x4318;
}

void exec(const char *path) {

    draw_printf(DBG_INFO, "LOADING: %s\n", path);

    int size = 0;
    char *bin = read_file(path, &size);
    if (bin == NULL || size < 1) {
        dash_log(DBG_ERROR, "EXEC: COULD NOT READ %s\n", path);
        return;
    }

    arch_exec(bin, size);
}

void exec_gz(const char *path, size_t size) {
    draw_printf(DBG_INFO, "LOADING: %s\n", path);

    char *bin = decompress_file(path, size);
    if (bin == NULL || size < 1) {
        dash_log(DBG_ERROR, "EXEC: COULD NOT READ %s\n", path);
        return;
    }

    arch_exec(bin, size);
}

void launch_retrodream(void) {
    if (file_exists("/sd/RD/retrodream.bin")) {
        exec("/sd/RD/retrodream.bin");
    } else if (file_exists("/ide/RD/retrodream.bin")) {
        exec("/ide/RD/retrodream.bin");
    }
}

void launch_dreamshell(void) {
    if (file_exists("/sd/DS/DS_CORE.BIN")) {
        exec("/sd/DS/DS_CORE.BIN");
    } else if (file_exists("/ide/DS/DS_CORE.BIN")) {
        exec("/ide/DS/DS_CORE.BIN");
    }
}

void launch_dcload_serial(void) {
    exec_gz("/rd/dcload-serial.bin.gz", 15776);
}

void launch_dcload_ip(void) {
    exec_gz("/rd/dcload-ip.bin.gz", 23736);
}

void loader_init() {
    InitIDE();
    InitSDCard();
}
