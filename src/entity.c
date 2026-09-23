#include "game.h"
#include "entity.h"
#include "sprite.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

entity_id entity_alloc(uint32_t type)
{
    for (ptrdiff_t i = 0; i < arrlen(obj); ++i) {
        if (!obj[i].is_active || obj[i].type == O_none) {
            obj[i] = entity_init(type);
            return i;
        }
    }

    arrput(obj, entity_init(type));
    return arrlen(obj) - 1;
}

struct entity *entity_get(entity_id id)
{
    if (id < 0 || id >= arrlen(obj)) return NULL;
    return &obj[id];
}

void entity_free(struct entity *o)
{
    o->is_active = 0;
    o->type = O_none;
}

struct entity entity_init(uint32_t type)
{
    struct entity new = {0};
    new.type = type;
    new.is_active = true;
    new.position = (Vector2) {0, 0};
    new.speed = (Vector2) {1, 0};
    new.rotation = 0.0f;
    return new;
}

uint32_t entity_set_action(struct entity *s, uint32_t action)
{
    s->position.x += s->speed.x;
    s->position.y += s->speed.y;
    s->rotation += 1;
    switch (action) {
        case KEY_SPACE:
            entity_act_shoot_missle(s);
            break;
        case KEY_W:
            entity_act_move_up(s);
            break;
        case KEY_S:
            entity_act_move_down(s);
            break;
        case KEY_A:
            entity_act_move_left(s);
            break;
        case KEY_D:
            entity_act_move_right(s);
            break;
    }
    return 0;
}

uint32_t entity_act_move_up(struct entity *s)
{
    if (s->speed.x > -2)
        if (s->speed.y > -2)
            s->speed.y -= 1;
    return 0;
}

uint32_t entity_act_move_down(struct entity *s)
{
    if (s->speed.x > -2)
        if (s->speed.y < 2)
            s->speed.y += 1;
    return 0;
}

uint32_t entity_act_shoot_missle(struct entity *s)
{
    // entity_alloc() may realloc obj, so copy everything needed from s first.
    Vector2 position = s->sp.position;

    entity_id id = entity_alloc(O_bullet);
    struct entity *o = entity_get(id);
    if (!o) return 0;

    o->position = (Vector2){
        rand_between(position.x, position.x + 300.0f),
        rand_between(position.y, position.y + 300.0f),
    };
    o->lifetime = rand_between(60, 180);
    return 1;
}

uint32_t entity_act_move_left(struct entity *s)
{
    if (s->speed.x > -2)
        s->speed.x -= 1;
    return 0;
}

uint32_t entity_act_move_right(struct entity *s)
{
    if (s->speed.x < 2)
        s->speed.x += 1;
    return 0;
}

void entity_update(struct entity *s)
{
    switch(s->type) {
        case O_player:
            // TODO 202609212108 create a entity_player_update()
            // to handle player logics/tick
            sprite_set_position(&s->sp, (Vector2){s->position.x, s->position.y});
            sprite_set_rotation(&s->sp, s->rotation);
            sprite_update(&s->sp);
            break;
        case O_bullet:

            // TODO 202609212109 have your own *_update() function
            if (s->lifetime > 0) --s->lifetime;

            if (rand_between(0, 1) > 0) {
                s->position.x += rand_between(4, 10);
            } else {
                s->position.x -= rand_between(4, 10);
            }
            if (rand_between(0, 1) > 0) {
                s->position.y += rand_between(4, 10);
            } else {
                s->position.y -= rand_between(4, 10);
            }

            DrawCircleV(s->position, 16.0f, YELLOW);
            DrawCircleV(s->position, 16.0f - 1, WHITE);

            if (s->lifetime <= 0) entity_free(s);

            break;
        case O_enemy:
            break;
    }
}
