#ifndef DREAMDASH_SCENE_H
#define DREAMDASH_SCENE_H

void disc_init(void);
void disc_enter(void);
void disc_draw(void);
void disc_input(void);

void filer_init(void);
void filer_enter(void);
void filer_enter_path(char *path);
void filer_draw(void);
void filer_input(void);

void log_init(void);
void log_enter(void);
void log_draw(void);
void log_input(void);

void mainmenu_init();
void mainmenu_enter();
void mainmenu_draw();
void mainmenu_input();

#endif //DREAMDASH_SCENE_H
