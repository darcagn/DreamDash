#ifndef DREAMDASH_DREAMDASH_H
#define DREAMDASH_DREAMDASH_H

typedef enum scene {
    SCENE_MAINMENU,
    SCENE_FILER,
    SCENE_LOG
} scene_t;

extern scene_t scene_id;
extern void (*scene_input_fn)(void);
extern void (*scene_draw_fn)(void);

#endif //DREAMDASH_DREAMDASH_H
