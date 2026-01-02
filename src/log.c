#include <kos/dbglog.h>
#include <kos/dbgio.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "drawing.h"
#include "utility.h"

List logList;

static ListItem* dash_log_new_line(void) {
    ListItem *item;

    item = (ListItem *) malloc(sizeof *item);
    memset(item, 0, sizeof(ListItem));
    item->type = TYPE_FILE;

    TAILQ_INSERT_TAIL(&logList.head, item, entries);
    logList.size++;

    return item;
}

static int dash_log_dbgio_detected(void) {
    return 1;
}

static int dash_log_dbgio_init(void) {
    logList = (List) {
        .head = TAILQ_HEAD_INITIALIZER(logList.head),
        .size = 0,
        .path = "LOGS"
    };

    return 0;
}

static int dash_log_dbgio_shutdown(void) {
    return 0;
}
static int dash_log_dbgio_set_irq_usage(int mode) {
    (void)mode;
    return 0;
}
static int dash_log_dbgio_read(void) {
    errno = EAGAIN;
    return -1;
}

static int dash_log_dbgio_write(int c) {
    /* Get the tail item from the list */
    ListItem *item = TAILQ_LAST(&logList.head, ListHead);
    if (!item) {
        /* No items in the list, create the first one */
        item = dash_log_new_line();
    }

    /* Create a new item for a new line */
    if (c == '\n') {
        item = dash_log_new_line();
        return 1;
    }

    /* Find current length of the string */
    size_t len = strlen(item->name);

    /* Check if there's room for one more character plus null terminator */
    if (len < MAX_PATH - 1) {
        item->name[len] = c;
        item->name[len + 1] = '\0';
    }

    return 1;
}

static int dash_log_dbgio_flush(void) {
    return 0;
}

static int dash_log_dbgio_write_buffer(const uint8_t *data, int len, int xlat) {
    (void)xlat;

    for (size_t i = 0; i < len; i++) {
        dash_log_dbgio_write(data[i]);
    }

    return len;
}

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

int dbgio_init(void) {
    dbgio_add_handler(&dash_log_dbgio);
    dbgio_dev_select("dash");
    dbgio_enable();

    return 0;
}
