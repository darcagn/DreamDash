#ifndef LOADER_UTILITY_H
#define LOADER_UTILITY_H

#define MAX_PATH 512

enum FileType {
    TYPE_DIR,
    TYPE_FILE,
    TYPE_BIN
};

typedef struct ListItem {
    struct ListItem *next, *prev;
    char name[MAX_PATH];
    char path[MAX_PATH];
    int type;
} ListItem;

typedef struct List {
    ListItem *head;
    int size;
    char path[MAX_PATH];
} List;

int list_cmp(ListItem *a, ListItem *b);

void get_dir(List *list, const char *path);

void free_dir(List *list);

ListItem *get_item(List *list, int index);

int file_exists(const char *file);

int dir_exists(const char *dir);

void try_boot();

char *read_file(const char *file, int *size);

void *decompress_file_aligned(const char *file, int alignment, int output_size);

void *decompress_file(const char *file, int output_size);

void exec(const char *path);

int is_hacked_bios();

int is_custom_bios();

int is_no_syscalls();

int flash_get_region();

void descramble(uint8_t *source, uint8_t *dest, uint32_t size);

int setup_syscalls();

void launch_retrodream();

void launch_dreamshell();

void launch_dcload_serial();

void launch_dcload_ip();

void loader_init();

void trim(char *str);

#endif //LOADER_UTILITY_H
