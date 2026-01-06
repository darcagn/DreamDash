#include <kos/init.h>

#include <dc/maple/controller.h>

#include "bg.h"
#include "disc.h"
#include "dreamdash.h"
#include "drawing.h"
#include "input.h"
#include "scene.h"
#include "storage.h"
#include "utility.h"

KOS_INIT_FLAGS(INIT_IRQ |INIT_THD_PREEMPT | INIT_FS_ALL | \
               INIT_LIBRARY | INIT_CDROM | INIT_CONTROLLER | INIT_VMU);

scene_t scene_id;
void (*scene_input_fn)(void);
void (*scene_draw_fn)(void);

int main(int argc, char **argv) {
    uint32_t keys = get_input();
    if (keys & CONT_A && keys & CONT_B) {
        launch_dcload_serial();
    } else if (keys & CONT_X && keys & CONT_Y) {
        launch_dcload_ip();
    }

    storage_init();

    draw_init();
    bg_init();

#ifdef AUTOBOOT
    if (!(keys & CONT_START)) {
        try_boot();
    }
#endif

#ifdef DISC_SUPPORT
    disc_init();
#endif
#ifndef DISABLE_LOGGER
    log_init();
#endif
    mainmenu_init();
    filer_init();

    mainmenu_enter();

    while (1) {
        scene_input_fn();
        draw_start();
        scene_draw_fn();
        bg_run();
        draw_end();
    }

    bg_exit();
    draw_exit();
    storage_exit();

    return 0;
}
