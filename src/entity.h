#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include "sprite.h"

enum {
    O_none,
    O_player,
    O_bullet,
    O_enemy,
};

struct entity {
    uint32_t type;
    bool is_active;
    Vector2 position;
    Vector2 speed;
    float rotation;
    struct sprite sp;
    uint32_t lifetime;
};

struct entity entity_init(uint32_t type);
void entity_update(struct entity *s);
uint32_t entity_set_action(struct entity *s, uint32_t action);
uint32_t entity_act_move_up(struct entity *s);
uint32_t entity_act_move_down(struct entity *s);
uint32_t entity_act_move_left(struct entity *s);
uint32_t entity_act_move_right(struct entity *s);
uint32_t entity_act_shoot_missle(struct entity *s);

#endif //ENTITY_H
