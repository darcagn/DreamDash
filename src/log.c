#include <kos/dbglog.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "drawing.h"
#include "utility.h"

List logList;
static int log_initialized = 0;

void log_init(void) {
    if (!log_initialized) {
        logList = (List) {
            .head = TAILQ_HEAD_INITIALIZER(logList.head),
            .size = 0,
            .path = "LOGS"
        };

        log_initialized = 1;
    }
}

void dash_log(int level, const char *fmt, ...) {

    ListItem *item;
    va_list args;

    log_init();

    item = (ListItem *) malloc(sizeof *item);
    memset(item, 0, sizeof(ListItem));

    switch (level) {
        case DBG_DEAD:
        case DBG_CRITICAL:
        case DBG_ERROR:
            item->type = TYPE_DIR;
            break;
        case DBG_WARNING:
            item->type = TYPE_BIN;
            break;

        default:
            item->type = TYPE_FILE;
            break;
    }

    va_start(args, fmt);
    vsnprintf(item->name, MAX_PATH, fmt, args);
    va_end(args);

    TAILQ_INSERT_TAIL(&logList.head, item, entries);
    logList.size++;

    // debug to screen too
    draw_printf(level, item->name);
    // debug to console too
    dbglog(level, item->name);
}
