#include <kos.h>
#include "menu.h"
#include "input.h"
#include "utility.h"
#include "drawing.h"
#include "disc.h"
#include "log.h"

int main(int argc, char **argv) {
    dash_log(DBG_INFO, "%s", kos_get_banner());

    uint32_t keys = get_input();
    if (keys & CONT_A && keys & CONT_B) {
        launch_dcload_serial();
    } else if (keys & CONT_X && keys & CONT_Y) {
        launch_dcload_ip();
    }

    disc_init();
    draw_init();
    loader_init();
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

    return 0;
}
