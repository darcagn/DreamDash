#ifndef DREAMDASH_UTILITY_H
#define DREAMDASH_UTILITY_H

#include <sys/queue.h>

#define MAX_PATH 512

int file_exists(const char *file);

int dir_exists(const char *dir);

void try_boot();

char *read_file(const char *file, int *size);

void *decompress_file(const char *file, int output_size);

void exec(const char *path);

void launch_retrodream();

void launch_dreamshell();

void launch_dcload_serial();

void launch_dcload_ip();

void trim(char *str);

#endif //DREAMDASH_UTILITY_H
