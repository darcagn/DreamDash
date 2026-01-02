#include <kos.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <zlib/zlib.h>

#include "drawing.h"
#include "utility.h"

int list_cmp(ListItem *a, ListItem *b) {

    if (a->type == TYPE_DIR && b->type != TYPE_DIR) {
        return -1;
    } else if (a->type != TYPE_DIR && b->type == TYPE_DIR) {
        return 1;
    }

    return strcasecmp(a->name, b->name);
}

/* Sort the list using insertion sort */
static void list_sort(List *list) {
    ListItem *item, *sorted_item, *temp;
    struct ListHead sorted_head;

    if (list->size <= 1) {
        return;
    }

    TAILQ_INIT(&sorted_head);

    /* Move all items to sorted list in order */
    while (!TAILQ_EMPTY(&list->head)) {
        item = TAILQ_FIRST(&list->head);
        TAILQ_REMOVE(&list->head, item, entries);

        /* Find insertion point */
        if (TAILQ_EMPTY(&sorted_head)) {
            TAILQ_INSERT_HEAD(&sorted_head, item, entries);
        } else {
            sorted_item = NULL;
            TAILQ_FOREACH(temp, &sorted_head, entries) {
                if (list_cmp(item, temp) < 0) {
                    sorted_item = temp;
                    break;
                }
            }

            if (sorted_item != NULL) {
                TAILQ_INSERT_BEFORE(sorted_item, item, entries);
            } else {
                TAILQ_INSERT_TAIL(&sorted_head, item, entries);
            }
        }
    }

    /* Move sorted items back to original list */
    list->head = sorted_head;
}

void free_dir(List *list) {

    ListItem *elt, *tmp;
    TAILQ_FOREACH_SAFE(elt, &list->head, entries, tmp) {
        TAILQ_REMOVE(&list->head, elt, entries);
        free(elt);
    }
}

ListItem *get_item(List *list, int index) {

    ListItem *file;
    int i = 0;

    TAILQ_FOREACH(file, &list->head, entries) {
        if (i == index) {
            return file;
        }
        i++;
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
    TAILQ_INIT(&list->head);
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

            TAILQ_INSERT_TAIL(&list->head, entry, entries);
            list->size++;
        }

        list_sort(list);
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

void *decompress_file(const char *file, int output_size) {
    gzFile gzfile = gzopen(file, "rb");

    if(!gzfile) {
        dbglog(DBG_ERROR, "Error opening %s!", file);
        return NULL;
    }

    void *buffer = malloc(output_size);
    if(!buffer) {
        dbglog(DBG_ERROR, "Error in malloc!");
        return NULL;
    }

    if(gzread(gzfile, buffer, output_size) != output_size) {
        dbglog(DBG_ERROR, "Error decompressing %s!", file);
        return NULL;
    }

    gzclose(gzfile);

    return buffer;
}

void exec(const char *path) {

    draw_printf(DBG_INFO, "LOADING: %s\n", path);

    int size = 0;
    char *bin = read_file(path, &size);
    if (bin == NULL || size < 1) {
        dbglog(DBG_ERROR, "EXEC: COULD NOT READ %s\n", path);
        return;
    }

    arch_exec(bin, size);
}

void exec_gz(const char *path, size_t size) {
    draw_printf(DBG_INFO, "LOADING: %s\n", path);

    char *bin = decompress_file(path, size);
    if (bin == NULL || size < 1) {
        dbglog(DBG_ERROR, "EXEC: COULD NOT READ %s\n", path);
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
