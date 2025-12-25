#ifndef GAME_H
#define GAME_H

#include <stdint.h>

struct game_state {
    uint32_t fps;
    uint32_t display_width;
    uint32_t display_height;
    uint32_t current_scene;
    uint32_t display_should_close;
};

struct game_state game_init(void);
uint32_t game_update(void);
uint32_t game_key_down(void);

#endif //GAME_H
