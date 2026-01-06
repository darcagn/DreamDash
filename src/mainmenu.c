#include <kos.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

#include "dreamdash.h"
#include "disc.h"
#include "drawing.h"
#include "input.h"
#include "scene.h"
#include "utility.h"

/* TODO: Allow the creation of arbitrary entries here */

typedef enum mainmenu {
    MENU_RETRODREAM,
    MENU_DREAMSHELL,
    MENU_FILER,
    MENU_DCLOAD_SERIAL,
    MENU_DCLOAD_IP,
#ifdef DISC_SUPPORT
    MENU_DISC,
#endif
#ifndef DISABLE_LOGGER
    MENU_LOGS
#endif
} mainmenu_id_t;

static size_t line_display_max = 10;
static size_t list_index = 0;
static size_t highlight_index = 0;

/* GUI rects */
static rect_t menurect = {
    .left = 32,
    .top = 32,
    .width = DRAW_SCREEN_WIDTH - 64,
    .height = DRAW_SCREEN_HEIGHT - 64
};

/* TAILQ infrastructure for our main menu */
typedef TAILQ_HEAD(mainmenu_list_head, mainmenu_item) mainmenu_list_head_t;

typedef struct mainmenu_item {
    TAILQ_ENTRY(mainmenu_item) entries;
    mainmenu_id_t id;
    char text[MAX_PATH];
} mainmenu_item_t;

typedef struct mainmenu_list {
    mainmenu_list_head_t head;
    size_t size;
} mainmenu_list_t;

static mainmenu_list_t mainmenu;

/* Add a new entry to our main menu */
static void menu_main_add_item(const char *name, int menu_id) {
    mainmenu_item_t *item = calloc(1, sizeof *item);
    if (!item) {
        return;
    }

    item->id = menu_id;
    snprintf(item->text, MAX_PATH, "%s", name);
    TAILQ_INSERT_TAIL(&mainmenu.head, item, entries);
    mainmenu.size++;
}

/* Initialize our main menu */
void mainmenu_init(void) {
    mainmenu = (mainmenu_list_t) {
        .head = TAILQ_HEAD_INITIALIZER(mainmenu.head),
        .size = 0,
    };

    /* Look for RetroDream on SD or IDE storage and add it to the menu if found */
    if (file_exists("/sd/RD/retrodream.bin") || file_exists("/ide/RD/retrodream.bin")) {
        menu_main_add_item("RetroDream", MENU_RETRODREAM);
    }

    /* Look for DreamShell on SD or IDE storage and add it to the menu if found */
    if (file_exists("/sd/DS/DS_CORE.BIN") || file_exists("/ide/DS/DS_CORE.BIN")) {
        menu_main_add_item("DreamShell", MENU_DREAMSHELL);
    }

#ifdef DISC_SUPPORT
    menu_main_add_item("Play Disc", MENU_DISC);
#endif
    menu_main_add_item("File Browser", MENU_FILER);
    menu_main_add_item("dcload-ip", MENU_DCLOAD_IP);
    menu_main_add_item("dcload-serial", MENU_DCLOAD_SERIAL);
#ifndef DISABLE_LOGGER
    menu_main_add_item("View Logs", MENU_LOGS);
#endif
}

/* Enter the main menu scene */
void mainmenu_enter() {
    scene_id = SCENE_MAINMENU;
    scene_input_fn = mainmenu_input;
    scene_draw_fn = mainmenu_draw;

    list_index = 0;
    highlight_index = 0;
}

/* Get the menu item for the requested index */
static inline mainmenu_item_t *get_menu_item(size_t index) {
    mainmenu_item_t *item;

    TAILQ_FOREACH(item, &mainmenu.head, entries) {
        if (index-- == 0) {
            return item;
        }
    }

    return NULL;
}

void mainmenu_draw(void) {
    rect_t main_rect = (rect_t) {menurect.left + 30, menurect.top + 30,
                                 menurect.width - 380, (DRAW_FONT_HEIGHT + DRAW_FONT_LINE_SPACING + 2) * (float) mainmenu.size};

    draw_box_outline(main_rect.left, main_rect.top, main_rect.width, main_rect.height,
                     100, COL_ALPHA(COL_BLACK, 128), COL_ALPHA(COL_BLACK, 96), 4);

    for (size_t i = 0; i < line_display_max; i++) {

        if (list_index + i < mainmenu.size) {

            if (i == highlight_index) {
                draw_box_outline(main_rect.left, main_rect.top + ((float) (i * DRAW_LINE_HEIGHT)),
                                 main_rect.width, (float) DRAW_LINE_HEIGHT,
                                 102, COL_TRUE_BLUE, COL_WHITE, 2);
            }

            mainmenu_item_t *item = get_menu_item(list_index + i);
            if (item) {
                draw_string(main_rect.left + 4,
                            main_rect.top + (DRAW_FONT_LINE_SPACING / 2) + ((float) (i * DRAW_LINE_HEIGHT)) + 2,
                            103, COL_WHITE, item->text);
            }
        }
    }

    /* Print version */

    draw_string(17, DRAW_SCREEN_HEIGHT - DRAW_FONT_HEIGHT - 15,
                103, COL_BLACK, "DreamDash BIOS v"DASH_VERSION);

    draw_string(16, DRAW_SCREEN_HEIGHT - DRAW_FONT_HEIGHT - 16,
                103, COL_WHITE, "DreamDash BIOS v"DASH_VERSION);

    draw_string(17, DRAW_SCREEN_HEIGHT - DRAW_FONT_HEIGHT - 16,
                103, COL_WHITE, "DreamDash BIOS v"DASH_VERSION);
}

void mainmenu_input(void) {
    uint32_t input = get_input();
    size_t half_line = line_display_max / 2;
    size_t current_pos = list_index + highlight_index;

    if (input & CONT_DPAD_UP) {
        if (current_pos > 0) {                  /* Move upwards in the list */
            if (highlight_index > half_line || list_index == 0) {
                highlight_index--;              /* Move highlight cursor upwards */
            } else {
                list_index--;                   /* Scroll entire menu upwards */
            }
        } else {                                /* Wrap around to bottom */
            if (mainmenu.size > line_display_max) {
                list_index = mainmenu.size - line_display_max;
                highlight_index = line_display_max - 1;
            } else {
                list_index = 0;
                highlight_index = mainmenu.size - 1;
            }
        }
    }

    if (input & CONT_DPAD_DOWN) {
        if (current_pos < mainmenu.size - 1) {  /* Move downwards in the list */
            if (highlight_index < half_line || list_index + line_display_max >= mainmenu.size) {
                highlight_index++;              /* Move highlight cursor downwards */
            } else {
                list_index++;                   /* Scroll entire menu downwards */
            }
        } else {                                /* Wrap around to top */
            list_index = 0;
            highlight_index = 0;
        }
    }

    /* Handle menu selection */
    if (input & CONT_A) {
        mainmenu_item_t *item = get_menu_item(current_pos);
        if (item) {
            switch(item->id) {
                case MENU_FILER:
                    filer_enter();
                    break;

#ifndef DISABLE_LOGGER
                case MENU_LOGS:
                    log_enter();
                    break;
#endif

#ifdef DISC_SUPPORT
                case MENU_DISC:
                    disc_enter();
                    break;
#endif

                case MENU_RETRODREAM:
                    launch_retrodream();
                    break;

                case MENU_DREAMSHELL:
                    launch_dreamshell();
                    break;

                case MENU_DCLOAD_IP:
                    launch_dcload_ip();
                    break;

                case MENU_DCLOAD_SERIAL:
                    launch_dcload_serial();
                    break;

                default:
                    break;
            }
        }
    }
}
