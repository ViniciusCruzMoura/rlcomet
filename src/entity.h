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

// TODO 202610032046 add poly struct to do sat and mtv calc

struct entity {
    // TODO 202610012132 i think ill need a var to set if it is solid
    // or not, if its solid so activate collision or see obj type to
    // set the entity behaviour
    // bool is_solid;
    // TODO 202610012130 i need be able to make the object invisible
    // so i need var to set if its visible and change _draw behaviours
    // bool is_visible;
    // TODO 202609212114 type will predefine the object/sprite behaviour
    // so maybe is better use var name 'behaviour'
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

// TODO 202610012135 change all _init function that create
// a thing to _alloc cause it the true, when you create a
// instance of something a allocation was did
entity_id entity_alloc(uint32_t type);
struct entity *entity_get(entity_id id);
struct entity entity_init(uint32_t type); // TODO 202610012134 itll be static cause its private
void entity_update(struct entity *s);
void entity_free(struct entity *o);

// NOTE any object can have events related to 
// alloc, free, clean up, draw, mouse, keyboard, collision
// example, entity_event_key_down_left()
// i took those ideas from game maker

// NOTE any object can have signal like godot does
// like _on_body_entered, _on_body_exited, _on_area_2d_body_entered
// _on_animation_finished, _on_mouse_entered
// its depends on collision system, so it
// have a collision to make entity solid 
// and hae collision to detect if it trigger
// the signal

uint32_t entity_set_action(struct entity *s, uint32_t action);

// TODO 202610012136 static functions ? maybe
// uint32_t entity_act_move_up(struct entity *s);
// uint32_t entity_act_move_down(struct entity *s);
// uint32_t entity_act_move_left(struct entity *s);
// uint32_t entity_act_move_right(struct entity *s);
// uint32_t entity_act_shoot_missle(struct entity *s);

#endif //ENTITY_H
