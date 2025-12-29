#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include "sprite.h"

struct entity {
    bool is_active;
    Vector2 position;
    Vector2 speed;
    float rotation;
    struct sprite sp;
};

struct entity entity_init(void);
void entity_update(struct entity *s);
uint32_t entity_set_action(struct entity *s, uint32_t action);
uint32_t entity_act_move_up(struct entity *s);
uint32_t entity_act_move_down(struct entity *s);
uint32_t entity_act_move_left(struct entity *s);
uint32_t entity_act_move_right(struct entity *s);
uint32_t entity_act_shoot_missle(struct entity *s);

#endif //ENTITY_H
