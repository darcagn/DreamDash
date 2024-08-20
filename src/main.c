//
// Created by cpasjuste on 28/01/2020.
//

#include <kos.h>
#include "menu.h"
#include "input.h"
#include "utility.h"
#include "drawing.h"
#include "disc.h"

int main(int argc, char **argv) {

    uint32_t keys = get_input();
    if (keys & INPUT_A && keys & INPUT_B) {
        launch_dcload_serial();
    } else if (keys & INPUT_X && keys & INPUT_Y) {
        launch_dcload_ip();
    }

    disc_init();
    draw_init();
    loader_init();
    back_init();

#ifdef AUTOBOOT
    if (keys & INPUT_START) {
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
