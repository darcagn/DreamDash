#ifndef DREAMDASH_DISC_H
#define DREAMDASH_DISC_H

typedef struct ip_meta {
	char hardware_ID[16];
	char maker_ID[16];
	char ks[5];
	char disk_type[6];
	char disk_num[5];
	char country_codes[8];
	char ctrl[4];
	char dev[1];
	char VGA[1];
	char WinCE[1];
	char unk[1];
	char product_ID[10];
	char product_version[6];
	char release_date[8];
	char unk2[8];
	char boot_file[16];
	char software_maker_info[16];
	char title[128];
} ip_meta_t;

extern ip_meta_t *ip_info;

int disc_init(void);
void disc_shutdown(void);
void disc_launch(void);

#endif //DREAMDASH_DISC_H
