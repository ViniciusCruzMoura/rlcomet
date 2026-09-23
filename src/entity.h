#ifndef ENTITY_H
#define ENTITY_H

#include <stddef.h>
#include <stdint.h>
#include "sprite.h"

enum {
    O_none,
    O_player,
    O_bullet,
    O_enemy,
};

typedef ptrdiff_t entity_id;
#define ENTITY_NONE ((entity_id)-1)

struct entity {
    // TODO 202609212114 it will predefine the object/sprite behaviour
    // so maybe is better use var name behaviour
    uint32_t type;
    bool is_active;
    // TODO 202609212113 is_visible; it will be useful for event block
    Vector2 position;
    Vector2 speed;
    float rotation;
    // TODO 202609212138 if dont have sprite load a default one
    struct sprite sp;
    uint32_t lifetime;
};

entity_id entity_alloc(uint32_t type);
struct entity *entity_get(entity_id id);
void entity_free(struct entity *o);
struct entity entity_init(uint32_t type);
void entity_update(struct entity *s);
uint32_t entity_set_action(struct entity *s, uint32_t action);
uint32_t entity_act_move_up(struct entity *s);
uint32_t entity_act_move_down(struct entity *s);
uint32_t entity_act_move_left(struct entity *s);
uint32_t entity_act_move_right(struct entity *s);
uint32_t entity_act_shoot_missle(struct entity *s);

#endif //ENTITY_H
