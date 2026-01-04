#include <kos.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <zlib/zlib.h>

#include "drawing.h"
#include "utility.h"

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
        dbglog(DBG_ERROR, "read_file: can't open %s\n", file);
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
        dbglog(DBG_ERROR, "read_file: can't read %s\n", file);
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
        dbglog(DBG_ERROR, "Error opening %s!\n", file);
        return NULL;
    }

    void *buffer = malloc(output_size);
    if(!buffer) {
        dbglog(DBG_ERROR, "Error in malloc!\n");
        return NULL;
    }

    if(gzread(gzfile, buffer, output_size) != output_size) {
        dbglog(DBG_ERROR, "Error decompressing %s\n!", file);
        return NULL;
    }

    gzclose(gzfile);

    return buffer;
}

void exec(const char *path) {

    draw_printf("LOADING: %s\n", path);

    int size = 0;
    char *bin = read_file(path, &size);
    if (bin == NULL || size < 1) {
        dbglog(DBG_ERROR, "EXEC: COULD NOT READ %s\n", path);
        return;
    }

    arch_exec(bin, size);
}

void exec_gz(const char *path, size_t size) {
    draw_printf("LOADING: %s\n", path);

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
    exec_gz("/rd/dcload-serial.bin.gz", DCLOAD_SERIAL_BINSIZE);
}

void launch_dcload_ip(void) {
    exec_gz("/rd/dcload-ip.bin.gz", DCLOAD_IP_BINSIZE);
}
