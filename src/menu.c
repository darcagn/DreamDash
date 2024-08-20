//
// Created by cpasjuste on 28/01/2020.
//

#include <kos.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "uthash/utlist.h"
#include "log.h"
#include "input.h"
#include "drawing.h"
#include "utility.h"
#include "menu.h"
#include "disc.h"

static char dreamboot_version[MAX_PATH];
static int menu_id = MENU_MAIN;
static int line_height = 0;
static int line_max = 0;
static int list_index = 0;
static int highlight_index = 0;
static Rect menuRect;
static Rect pathRect;
static Rect filerRect;
static List menuList = {NULL, 0, "BOOT MENU"};
static List fileList;
extern List logList;

static void menu_main_add_item(const char *name, int menu_id) {
    ListItem *item = (ListItem *) malloc(sizeof *item);
    memset(item, 0, sizeof(ListItem));
    item->type = menu_id;
    strcpy(item->name, name);
    DL_APPEND(menuList.head, item);
    menuList.size++;
}

static void menu_init() {

    Vec2 screenSize = draw_get_screen_size();

    menuRect = (Rect) {32, 32, screenSize.x - 64, screenSize.y - 64};

    pathRect = (Rect) {menuRect.left + 8, menuRect.top + 8,
                       menuRect.width - 16, DRAW_FONT_HEIGHT + 16};

    filerRect = (Rect) {menuRect.left + 8, pathRect.top + pathRect.height + 10,
                        menuRect.width - 16, menuRect.height - pathRect.height - 26};

    line_height = DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING;
    line_max = (int) filerRect.height / line_height;
    if (line_max * line_height < (int) filerRect.height) {
        line_height = (int) filerRect.height / line_max;
    }

    // build main menu
    if (file_exists("/sd/RD/retrodream.bin") || file_exists("/ide/RD/retrodream.bin")) {
        menu_main_add_item("RetroDream", MENU_RETRODREAM);
    }
    if (file_exists("/sd/DS/DS_CORE.BIN") || file_exists("/ide/DS/DS_CORE.BIN")) {
        menu_main_add_item("DreamShell", MENU_DREAMSHELL);
    }
    menu_main_add_item("File Browser", MENU_FILER);
    menu_main_add_item("dcload-ip", MENU_DCLOAD_IP);
    menu_main_add_item("dcload-serial", MENU_DCLOAD_SERIAL);
    menu_main_add_item("Play Disc", MENU_DISC);
    menu_main_add_item("View Logs", MENU_LOGS);

    sprintf(dreamboot_version, "DREAMBOOT v%s @ CPASJUSTE", __DB_VERSION__);
}

static void menu_get_dir(char *path) {

    list_index = 0;
    highlight_index = 0;

    free_dir(&fileList);
    get_dir(&fileList, path);
}

static List *menu_get_list() {

    List *list = &menuList;
    if (menu_id == MENU_FILER) {
        list = &fileList;
    } else if (menu_id == MENU_LOGS) {
        list = &logList;
    }
    return list;
}

static void menu_draw_main() {

    List *list = &menuList;

    Rect mainRect = (Rect) {menuRect.left + 30, menuRect.top + 30,
                            menuRect.width - 380, (DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING + 2) * (float) list->size};

    draw_box_outline(mainRect.left, mainRect.top, mainRect.width, mainRect.height,
                     100, COL_BLACK_TRANS1, COL_BLACK_TRANS2, 4);

    for (int i = 0; i < (unsigned int) line_max; i++) {

        if (list_index + i < list->size) {

            if (i == highlight_index) {
                draw_box_outline(mainRect.left, mainRect.top + ((float) (i * line_height)),
                                 mainRect.width, (float) line_height,
                                 102, COL_TRUE_BLUE, COL_WHITE, 2);
            }

            ListItem *item = get_item(list, list_index + i);
            if (item != NULL) {
                draw_string(mainRect.left + 4,
                            mainRect.top + (DRAW_FONT_LINE_SPACING / 2) + ((float) (i * line_height)) + 2,
                            103, COL_WHITE, item->name);
            }
        }
    }

    // version
    draw_string(16, draw_get_screen_size().y - DRAW_FONT_HEIGHT - 16,
                103, COL_TRUE_BLUE, "Dreamboot BIOS v"__DB_VERSION__);
}

static void menu_draw() {

    if (menu_id == MENU_MAIN) {
        menu_draw_main();
        return;
    }

    List *list = menu_get_list();

    draw_box_outline(pathRect.left, pathRect.top, pathRect.width, pathRect.height,
                     100, COL_BLACK_TRANS1, COL_BLACK_TRANS2, 4);
    draw_string(pathRect.left + 4, pathRect.top + (DRAW_FONT_LINE_SPACING / 2) + 6, 103, COL_TRUE_BLUE, list->path);

    draw_box_outline(filerRect.left, filerRect.top, filerRect.width, filerRect.height,
                     100, COL_BLACK_TRANS1, COL_BLACK_TRANS2, 4);

    for (int i = 0; i < (unsigned int) line_max; i++) {

        if (list_index + i < list->size) {

            if (menu_id == MENU_FILER && i == highlight_index) {
                draw_box_outline(filerRect.left, filerRect.top + ((float) (i * line_height)),
                                 filerRect.width, (float) line_height,
                                 102, COL_TRUE_BLUE, COL_WHITE, 2);
            }

            ListItem *item = get_item(list, list_index + i);
            if (item != NULL) {
                Color color = COL_WHITE;
                if ((menu_id == MENU_FILER && item->type == TYPE_DIR)
                    || (menu_id == MENU_LOGS && item->type == TYPE_BIN)) {
                    color = COL_YELLOW;
                } else if (menu_id == MENU_LOGS && item->type == TYPE_DIR) {
                    color = COL_RED;
                }
                draw_string(filerRect.left + 4,
                            filerRect.top + (DRAW_FONT_LINE_SPACING / 2) + ((float) (i * line_height)),
                            103, color, item->name);
            }
        }
    }
}

static int menu_input() {

    uint32 input = get_input();
    List *list = menu_get_list();

    if (input & INPUT_QUIT) {
        return INPUT_QUIT;
    }

    if (input & INPUT_UP) {
        if (highlight_index <= line_max / 2 && list_index > 0) {
            list_index--;
        } else {
            highlight_index--;
            if (highlight_index < 0) {
                highlight_index = line_max / 2;
                if (highlight_index >= list->size) {
                    highlight_index = list->size - 1;
                    list_index = 0;
                } else {
                    list_index = (list->size - 1) - highlight_index;
                }
            }
        }
    } else if (input & INPUT_DOWN) {
        if (highlight_index >= line_max / 2) {
            list_index++;
            if (list_index + highlight_index >= list->size) {
                list_index = 0;
                highlight_index = 0;
            }
        } else {
            highlight_index++;
            if (highlight_index >= list->size) {
                highlight_index = 0;
            }
        }
    }

    if (menu_id == MENU_MAIN) {
        if (input & INPUT_A) {
            ListItem *item = get_item(list, list_index + highlight_index);
            if (item != NULL) {
                if (item->type == MENU_FILER) {
                    menu_id = MENU_FILER;
                    menu_get_dir("/");
                } else if (item->type == MENU_LOGS) {
                    list_index = 0;
                    highlight_index = 0;
                    menu_id = MENU_LOGS;
                } else if (item->type == MENU_DISC) {
                    if (ip_info != NULL) {
                        disc_launch();
                    }
                } else if (item->type == MENU_RETRODREAM) {
                    launch_retrodream();
                } else if (item->type == MENU_DREAMSHELL) {
                    launch_dreamshell();
                } else if (item->type == MENU_DCLOAD_IP) {
                    launch_dcload_ip();
                } else if (item->type == MENU_DCLOAD_SERIAL) {
                    launch_dcload_serial();
                }
            }
        }
    } else if (menu_id == MENU_LOGS) {
        if (input & INPUT_B) {
            list_index = 0;
            highlight_index = 0;
            menu_id = MENU_MAIN;
        }
    } else if (menu_id == MENU_FILER) {
        if (input & INPUT_A) {
            ListItem *file = get_item(list, list_index + highlight_index);
            if (file != NULL) {
                if (file->type == TYPE_DIR) {
                    menu_get_dir(file->path);
                } else if (file->type == TYPE_BIN) {
                    exec(file->path);
                }
            }
        } else if (input & INPUT_B) {
            if (strlen(list->path) > 1) {
                char *pos = strrchr(list->path, '/');
                if (pos != NULL) {
                    if (strlen(pos) == 1) {
                        menu_get_dir("/");
                    } else {
                        char prev[MAX_PATH];
                        memset(prev, 0, MAX_PATH);
                        strncpy(prev, list->path, strlen(list->path) - strlen(pos) + 1);
                        menu_get_dir(prev);
                    }
                }
            } else {
                list_index = 0;
                highlight_index = 0;
                menu_id = MENU_MAIN;
            }
        }
    }

    return 0;
}

void menu_run() {

    uint32 input = 0;

    menu_init();

    while (1) {

        input = menu_input();
        if (input & INPUT_QUIT) {
            break;
        }

        draw_start();

        menu_draw();

        draw_back();

        draw_end();
    }
}
