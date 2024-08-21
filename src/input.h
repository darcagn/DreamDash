#ifndef LOADER_INPUT_H
#define LOADER_INPUT_H

#define BIT(n) (1U<<(n))

#define INPUT_QUIT  BIT(20)
#define INPUT_LEFT  CONT_DPAD_LEFT
#define INPUT_RIGHT CONT_DPAD_RIGHT
#define INPUT_UP    CONT_DPAD_UP
#define INPUT_DOWN  CONT_DPAD_DOWN
#define INPUT_A     CONT_A
#define INPUT_B     CONT_B
#define INPUT_X     CONT_X
#define INPUT_Y     CONT_Y
#define INPUT_START CONT_START

uint32_t get_input();

#endif //LOADER_INPUT_H
