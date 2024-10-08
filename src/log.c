#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "uthash/utlist.h"
#include "utility.h"
#include "drawing.h"

List logList = {NULL, 0, "LOGS"};

void dash_log(int level, const char *fmt, ...) {

    ListItem *item;
    va_list args;

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

    DL_APPEND(logList.head, item);
    logList.size++;

    // debug to screen too
    draw_printf(level, item->name);
}
