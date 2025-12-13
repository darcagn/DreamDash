#ifndef LOADER_MENU_H
#define LOADER_MENU_H

enum Menu {
    MENU_MAIN,
    MENU_RETRODREAM,
    MENU_DREAMSHELL,
    MENU_FILER,
    MENU_DCLOAD_SERIAL,
    MENU_DCLOAD_IP,
#ifdef DISC_SUPPORT
    MENU_DISC,
#endif
    MENU_LOGS
};

void menu_run();

#endif //LOADER_MENU_H
