#ifndef ENTITY_H
#define ENTITY_H

#include <stddef.h>
#include <stdint.h>
#include "sprite.h"
#include "physics.h"

enum {
    NONE,
    PLAYER,
    BULLET,
    ENEMY,
};

enum {
    PLAYER_IDLE,
    PLAYER_HAND_ATTACK,
    PLAYER_JUMP,
    PLAYER_DEATH,
    PLAYER_GUN_ATTACK,
    PLAYER_WALK,
    PLAYER_ANIMATION_COUNT
};

typedef ptrdiff_t entity_id;
#define ENTITY_NONE ((entity_id)-1)

struct level;

struct entity {
    // TODO 202609212114 type will predefine the object/sprite behaviour
    // so maybe is better use var name 'behaviour'
    uint32_t type;
    uint32_t layer;
    bool is_active;
    bool is_visible;
    bool unique;
    bool is_solid;
    int32_t health;
    uint32_t damage;
    uint64_t id;
    uint64_t owner_id;
    Vector2 facing;
    Vector2 position;
    Vector2 speed;
    struct sprite sp;
    struct physics_polygon collider;
    uint32_t lifetime;
};

// TODO 202610012135 change all _init function that create
// a thing to _alloc cause it the true, when you create a
// instance of something a allocation was did
struct entity entity_init(uint32_t type);
struct entity entity_bullet_init(void);
struct entity entity_copy(const struct entity *src);
void entity_update(struct entity *s);
void entity_draw(const struct entity *s);
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

#endif //ENTITY_H
