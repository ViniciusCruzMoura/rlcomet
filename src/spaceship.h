#ifndef SPACESHIP_H
#define SPACESHIP_H

#include <stdint.h>
#include "sprite.h"

struct spaceship_state {
    int32_t x, y; // position on scene
    uint32_t width, height; // spacecraft size
    int32_t acce_x, acce_y; // spacecraft acceleration
    float rotation;
    struct sprite sp;
};

struct spaceship_state spaceship_init(void);
uint32_t spaceship_set_action(struct spaceship_state *s, uint32_t action);
uint32_t spaceship_act_move_up(struct spaceship_state *s);
uint32_t spaceship_act_move_down(struct spaceship_state *s);
uint32_t spaceship_act_move_left(struct spaceship_state *s);
uint32_t spaceship_act_move_right(struct spaceship_state *s);
uint32_t spaceship_act_shoot_missle(struct spaceship_state *s);

#endif //SPACESHIP_H
