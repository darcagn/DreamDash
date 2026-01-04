#include <kos/dbglog.h>
#include <kos/dbgio.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "dreamdash.h"
#include "drawing.h"
#include "input.h"
#include "scene.h"
#include "utility.h"

/* TODO: Limit the number of logs, else we run out of memory */
/* TODO: Limit the width of the displayed logs, creating newlines as necessary */

/* Maximum number of lines the console can display */
static size_t line_display_max;

/* Current line index displayed */
static size_t line_index;

/* Header rectangle */
static rect_t head_rect = {
    .left = 40,
    .top = 40,
    .width = DRAW_SCREEN_WIDTH - 80,
    .height = DRAW_FONT_HEIGHT + 16
};

/* Main logger body rectangle */
static rect_t log_rect = {
    .left = 40,
    .top = DRAW_FONT_HEIGHT + 66,
    .width = DRAW_SCREEN_WIDTH - 80,
    .height = DRAW_SCREEN_HEIGHT - DRAW_FONT_HEIGHT - 96
};

/* TAILQ infrastructure for our logger */
typedef TAILQ_HEAD(log_list_head, log_line) log_list_head_t;

typedef struct log_line {
    TAILQ_ENTRY(log_line) entries;
    char text[MAX_PATH];
} log_line_t;

typedef struct log_list {
    log_list_head_t head;
    size_t size;
    char text[MAX_PATH];
} log_list_t;

static log_list_t dash_log;

/* Initialize the log scene. As of now, not required */
void log_init() {
    line_display_max = log_rect.height / DRAW_LINE_HEIGHT;
}

/* Get the line for the requested index */
static inline log_line_t *get_line(size_t index) {
    log_line_t *line;

    TAILQ_FOREACH(line, &dash_log.head, entries) {
        if (index-- == 0) {
            return line;
        }
    }

    return NULL;
}

void log_draw() {
    /* Draw header rectangle */
    draw_box_outline(head_rect.left, head_rect.top, head_rect.width, head_rect.height,
                     100, COL_ALPHA(COL_BLACK, 128), COL_ALPHA(COL_BLACK, 96), 4);

    /* Draw title */
    draw_string(head_rect.left + 4, head_rect.top + (DRAW_FONT_LINE_SPACING / 2) + 6, 103, COL_TRUE_BLUE, dash_log.text);

    /* Draw log body rectangle */
    draw_box_outline(log_rect.left, log_rect.top, log_rect.width, log_rect.height,
                     100, COL_ALPHA(COL_BLACK, 128), COL_ALPHA(COL_BLACK, 96), 4);

    /* Draw all text lines */
    for (size_t i = 0; i < line_display_max; i++) {
        if (line_index + i < dash_log.size) {
            log_line_t *line = get_line(line_index + i);
            if (line) {
                draw_string(log_rect.left + 4,
                            log_rect.top + (DRAW_FONT_LINE_SPACING / 2) + ((float) (i * DRAW_LINE_HEIGHT)),
                            103, COL_WHITE, line->text);
            }
        }
    }
}

void log_input(void) {
    uint32_t input = get_input();

    if (input & CONT_DPAD_UP) {
        if (line_index > 0) {
            line_index--;
        } else {
            if (dash_log.size > line_display_max) {
                line_index = dash_log.size - line_display_max;
            } else {
                line_index = 0;
            }
        }
    }

    if (input & CONT_DPAD_DOWN) {
        if (dash_log.size > line_display_max) {
            if (line_index >= dash_log.size - line_display_max) {
                line_index = 0;
            } else {
                line_index++;
            }
        } else {
            line_index = 0;
        }
    }

    if (input & CONT_B) {
        mainmenu_enter();
    }
}

/* Enter the log scene. */
void log_enter() {
    scene_id = SCENE_LOG;
    scene_input_fn = log_input;
    scene_draw_fn = log_draw;

    /* Set line_index so that the entire screen
       shows the tail end of the logs */
    if (dash_log.size > line_display_max) {
        line_index = dash_log.size - line_display_max;
    } else {
        line_index = 0;
    }
}

/* Insert a new line of text into the TAILQ */
static log_line_t* dash_log_new_line(void) {
    log_line_t *line = calloc(1, sizeof *line);

    if (!line) {
        return NULL;
    }

    TAILQ_INSERT_TAIL(&dash_log.head, line, entries);
    dash_log.size++;

    return line;
}

/* Always detect as valid dbgio interface */
static int dash_log_dbgio_detected(void) {
    return 1;
}

static int dash_log_dbgio_init(void) {
    dash_log = (log_list_t) {
        .head = TAILQ_HEAD_INITIALIZER(dash_log.head),
        .size = 0,
        .text = "KallistiOS / DreamDash Logs"
    };

    return 0;
}

/* TODO: We should be able to shut this down */
static int dash_log_dbgio_shutdown(void) {
    return 0;
}

/* FIXME */
static int dash_log_dbgio_set_irq_usage(int mode) {
    (void)mode;
    return 0;
}

/* We don't support this */
static int dash_log_dbgio_read(void) {
    errno = EAGAIN;
    return -1;
}

static int dash_log_dbgio_write(int c) {
    /* Get the tail item from the list */
    log_line_t *line = TAILQ_LAST(&dash_log.head, log_list_head);
    if (!line) {
        /* No items in the list, create the first one */
        line = dash_log_new_line();
    }

    /* Create a new item for a new line */
    if (c == '\n') {
        line = dash_log_new_line();
        return 1;
    }

    /* Find current length of the string */
    size_t len = strlen(line->text);

    /* Check if there's room for one more character plus null terminator */
    if (len < MAX_PATH - 1) {
        line->text[len] = c;
        line->text[len + 1] = '\0';
    }

    return 1;
}

/* Flush unneeded here */
static int dash_log_dbgio_flush(void) {
    return 0;
}

/* Iterate over the buffer using
   our character writing code */
static int dash_log_dbgio_write_buffer(const uint8_t *data, int len, int xlat) {
    (void)xlat;

    for (size_t i = 0; i < len; i++) {
        dash_log_dbgio_write(data[i]);
    }

    return len;
}

/* We don't support this */
static int dash_log_dbgio_read_buffer(uint8_t * data, int len) {
    (void)data;
    (void)len;
    errno = EAGAIN;
    return -1;
}

dbgio_handler_t dash_log_dbgio = {
    .name = "dash",
    .detected = dash_log_dbgio_detected,
    .init = dash_log_dbgio_init,
    .shutdown = dash_log_dbgio_shutdown,
    .set_irq_usage = dash_log_dbgio_set_irq_usage,
    .read = dash_log_dbgio_read,
    .write = dash_log_dbgio_write,
    .flush = dash_log_dbgio_flush,
    .write_buffer = dash_log_dbgio_write_buffer,
    .read_buffer = dash_log_dbgio_read_buffer
};

/* This overrides dbgio_init() in KallistiOS,
   so all logs will end up in DreamDash */
int dbgio_init(void) {
    dbgio_add_handler(&dash_log_dbgio);
    dbgio_dev_select("dash");
    dbgio_enable();

    return 0;
}
