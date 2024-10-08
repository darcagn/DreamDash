#include <kos.h>

static uint32_t last_buttons = 0;

uint32_t get_input() {

    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    if (cont == NULL) {
        return 0;
    }

    cont_state_t *state = (cont_state_t *) maple_dev_status(cont);
    if (state == NULL) {
        return 0;
    }

    if (last_buttons != state->buttons) {
        last_buttons = state->buttons;
        return state->buttons;
    }

    return 0;
}
