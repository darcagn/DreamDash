#include <kos/init.h>

#include <dc/maple/controller.h>

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

#ifdef DISC_SUPPORT
    disc_init();
#endif

    draw_init();
    back_init();

#ifdef AUTOBOOT
    if (!(keys & CONT_START)) {
        try_boot();
    }
#endif

    mainmenu_init();
    log_init();
    filer_init();

    mainmenu_enter();

    while (1) {
        scene_input_fn();
        draw_start();
        scene_draw_fn();
        draw_back();
        draw_end();
    }

    draw_exit();
    storage_shutdown();

    return 0;
}
