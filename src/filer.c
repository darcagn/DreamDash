#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kos/dbglog.h>

#include "dreamdash.h"
#include "drawing.h"
#include "input.h"
#include "scene.h"
#include "utility.h"

/* TODO: Improved sorting algorithm */

/* File types recognized by the filer */
typedef enum file_type {
    TYPE_DIR,
    TYPE_FILE,
    TYPE_BIN
} file_type_t;

/* TAILQ infrastructure for our filer */

/* Filer list head */
typedef TAILQ_HEAD(filer_list_head, filer_item) filer_list_head_t;

/* Filer list item */
typedef struct filer_item {
    TAILQ_ENTRY(filer_item) entries;
    char path[MAX_PATH];
    char name[MAX_PATH];
    file_type_t type;
} filer_item_t;

/* Filer list */
typedef struct filer_list {
    filer_list_head_t head;
    char path[MAX_PATH];
    size_t size;
} filer_list_t;

/* Final filer list TAILQ structure */
static filer_list_t filer;

static rect_t path_rect;
static rect_t filer_rect;

/* Menu trackers */
static size_t line_display_max;
static size_t list_index = 0;
static size_t highlight_index = 0;

/* Compare two filer items for sorting, with
   directories being sorted ahead of files */
static int list_cmp(filer_item_t *a, filer_item_t *b) {
    if (a->type == TYPE_DIR && b->type != TYPE_DIR) {
        return -1;
    } else if (a->type != TYPE_DIR && b->type == TYPE_DIR) {
        return 1;
    }

    return strcasecmp(a->name, b->name);
}

/* Sort the list using insertion sort */
static void list_sort(void) {
    filer_item_t *item, *sorted_item, *temp;
    filer_list_head_t sorted_head;

    if (filer.size <= 1) {
        return;
    }

    TAILQ_INIT(&sorted_head);

    /* Move all items to sorted list in order */
    while (!TAILQ_EMPTY(&filer.head)) {
        item = TAILQ_FIRST(&filer.head);
        TAILQ_REMOVE(&filer.head, item, entries);

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
    filer.head = sorted_head;
}

/* Empty and free all memory used for the filer */
static void filer_empty(void) {
    filer_item_t *elt, *tmp;
    TAILQ_FOREACH_SAFE(elt, &filer.head, entries, tmp) {
        TAILQ_REMOVE(&filer.head, elt, entries);
        free(elt);
    }
}

/* Get an item from from the filer list by index */
static filer_item_t *get_file(size_t index) {
    filer_item_t *file;

    TAILQ_FOREACH(file, &filer.head, entries) {
        if (index-- == 0) {
            return file;
        }
    }

    return NULL;
}

void filer_init(void) {
    rect_t menurect_t = (rect_t) {32, 32, DRAW_SCREEN_WIDTH - 64, DRAW_SCREEN_HEIGHT - 64};

    path_rect = (rect_t) {menurect_t.left + 8, menurect_t.top + 8,
                       menurect_t.width - 16, DRAW_FONT_HEIGHT + 16};

    filer_rect = (rect_t) {menurect_t.left + 8, path_rect.top + path_rect.height + 10,
                        menurect_t.width - 16, menurect_t.height - path_rect.height - 26};

    line_display_max = filer_rect.height / DRAW_LINE_HEIGHT;
}

/* Used to get dir info for filer menus */
static void filer_get_dir(char *path) {
    /* Reset navigation state */
    list_index = 0;
    highlight_index = 0;

    /* Clear and recreate our filer */
    filer_empty();
    TAILQ_INIT(&filer.head);
    filer.size = 0;
    snprintf(filer.path, MAX_PATH, "%s", path);

    dirent_t *ent;
    file_t fd;
    filer_item_t *entry;
    if ((fd = fs_open(path, O_RDONLY | O_DIR)) != FILEHND_INVALID) {
        while ((ent = fs_readdir(fd)) != NULL) {

            /* skip "." */
            if (ent->name[0] == '.') {
                continue;
            }

            /* Skip irrelevant devices */
            if (strncmp(ent->name, "dev", 3) == 0 ||
                strncmp(ent->name, "pty", 3) == 0 ||
                strncmp(ent->name, "ram", 3) == 0 )
            {
                continue;
            }

            size_t path_len = strlen(filer.path);
            size_t name_len = strlen(ent->name);

            /* Skip entries that would create paths too long.
               The 2 accounts for the / separator and the terminator */
            if (path_len + name_len + 2 >= MAX_PATH) {
                continue;
            }

            entry = calloc(1, sizeof *entry);
            if (!entry) {
                dbglog(DBG_ERROR, "Error allocating mem in %s()!\n", __func__);
                fs_close(fd);
                break;
            }

            snprintf(entry->name, MAX_PATH, "%s", ent->name);

            /* Build path - we've already verified it fits */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
            if (path_len > 0 && filer.path[path_len - 1] != '/') {
                snprintf(entry->path, MAX_PATH, "%s/%s", filer.path, ent->name);
            } else {
                snprintf(entry->path, MAX_PATH, "%s%s", filer.path, ent->name);
            }
#pragma GCC diagnostic pop

            entry->type = ent->attr == O_DIR ? TYPE_DIR : TYPE_FILE;
            if (entry->type == TYPE_FILE) {
                char *ext = strrchr(entry->name, '.');
                if (ext && (strcasecmp(ext, ".bin") == 0 || strcasecmp(ext, ".elf") == 0)) {
                    entry->type = TYPE_BIN;
                }
            }

            TAILQ_INSERT_TAIL(&filer.head, entry, entries);
            filer.size++;
        }

        list_sort();
        fs_close(fd);
    } else {
        dbglog(DBG_ERROR, "Error opening file %s!\n", path);
    }
}

void filer_draw(void) {
    draw_box_outline(path_rect.left, path_rect.top, path_rect.width, path_rect.height,
                     100, COL_ALPHA(COL_BLACK, 128), COL_ALPHA(COL_BLACK, 96), 4);
    draw_string(path_rect.left + 4, path_rect.top + (DRAW_FONT_LINE_SPACING / 2) + 6, 103, COL_TRUE_BLUE, filer.path);

    draw_box_outline(filer_rect.left, filer_rect.top, filer_rect.width, filer_rect.height,
                     100, COL_ALPHA(COL_BLACK, 128), COL_ALPHA(COL_BLACK, 96), 4);

    for (size_t i = 0; i < line_display_max; i++) {

        if (list_index + i < filer.size) {

            if (i == highlight_index) {
                draw_box_outline(filer_rect.left, filer_rect.top + ((float) (i * DRAW_LINE_HEIGHT)),
                                 filer_rect.width, (float) DRAW_LINE_HEIGHT,
                                 102, COL_TRUE_BLUE, COL_WHITE, 2);
            }

            filer_item_t *item = get_file(list_index + i);
            if (item != NULL) {
                color_t color = COL_WHITE;
                if (item->type == TYPE_DIR) {
                    color = COL_YELLOW;
                }

                draw_string(filer_rect.left + 4,
                            filer_rect.top + (DRAW_FONT_LINE_SPACING / 2) + ((float) (i * DRAW_LINE_HEIGHT)),
                            103, color, item->name);
            }
        }
    }
}

void filer_input(void) {
    uint32_t input = get_input();

    if (input & CONT_B) {
        if (strlen(filer.path) > 1) {
            char *pos = strrchr(filer.path, '/');
            if (pos && pos != filer.path) {
                char prev[MAX_PATH];
                snprintf(prev, pos - filer.path + 1, "%s", filer.path);
                filer_get_dir(prev);
            } else {
                filer_get_dir("/");
            }
        } else {
            mainmenu_enter();
        }
    }

    /* Empty directory - only back button works */
    if (filer.size == 0) {
        return;
    }

    size_t half_line = line_display_max / 2;
    size_t current_pos = list_index + highlight_index;

    if (input & CONT_A) {
        filer_item_t *file = get_file(current_pos);
        if (file) {
            if (file->type == TYPE_DIR) {
                filer_get_dir(file->path);
            } else if (file->type == TYPE_BIN) {
                exec(file->path);
            }
        }
    }

    if (input & CONT_DPAD_UP) {
        if (current_pos > 0) {                  /* Move upwards in the list */
            if (highlight_index > half_line || list_index == 0) {
                highlight_index--;              /* Move highlight cursor upwards */
            } else {
                list_index--;                   /* Scroll entire menu upwards */
            }
        } else {                                /* Wrap around to bottom */
            if (filer.size > line_display_max) {
                list_index = filer.size - line_display_max;
                highlight_index = line_display_max - 1;
            } else {
                list_index = 0;
                highlight_index = filer.size - 1;
            }
        }
    }

    if (input & CONT_DPAD_DOWN) {
        if (current_pos < filer.size - 1) {     /* Move downwards in the list */
            if (highlight_index < half_line || list_index + line_display_max >= filer.size) {
                highlight_index++;              /* Move highlight cursor downwards */
            } else {
                list_index++;                   /* Scroll entire menu downwards */
            }
        } else {                                /* Wrap around to top */
            list_index = 0;
            highlight_index = 0;
        }
    }
}

/* Enters the filer scene with a specific path */
void filer_enter_path(char *path) {
    scene_id = SCENE_FILER;
    scene_input_fn = filer_input;
    scene_draw_fn = filer_draw;

    filer_get_dir(path);
}

/* Enters the filer scene at the KOS filesystem root */
void filer_enter(void) {
    filer_enter_path("/");
}
