#include <kos/init.h>

#include <dc/maple/controller.h>

#include "disc.h"
#include "drawing.h"
#include "input.h"
#include "menu.h"
#include "storage.h"
#include "utility.h"

KOS_INIT_FLAGS(INIT_IRQ |INIT_THD_PREEMPT | INIT_FS_ALL | \
               INIT_LIBRARY | INIT_CDROM | INIT_CONTROLLER | INIT_VMU);

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
    if (keys & CONT_START) {
        menu_run();
    } else {
        try_boot();
        menu_run();
    }
#else
    menu_run();
#endif

    draw_exit();

    storage_shutdown();

    return 0;
}
