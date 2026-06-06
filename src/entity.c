#include "game.h"
#include "entity.h"
#include "sprite.h"
#include "common.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

struct entity entity_init(uint32_t type)
{
    return (struct entity) {
        type,
        true,
        (Vector2) {0, 0},
        (Vector2) {1, 0},
        .rotation = 0.0f,
    };
}

uint32_t entity_set_action(struct entity *s, uint32_t action)
{
    s->position.x += s->speed.x;
    s->position.y += s->speed.y;
    s->rotation += 1;
    entity_act_shoot_missle(s);
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
    int max_bullets = 0;
    objid i;
    for (i = 1; i < max_obj; ++i) {
        if (obj[i].type != O_bullet) continue;

        ++max_bullets;

        struct entity *o = &obj[i];

        if (o->lifetime > 0) --o->lifetime;

        if (rand_between(0, 1) > 0) {
            o->position.x += rand_between(4, 10);
        } else {
            o->position.x -= rand_between(4, 10);
        }

        if (rand_between(0, 1) > 0) {
            o->position.y += rand_between(4, 10);
        } else {
            o->position.y -= rand_between(4, 10);
        }

        DrawCircleV(o->position, 16.0f, YELLOW);
        DrawCircleV(o->position, 16.0f - 1, WHITE);

        if (o->lifetime <= 0) free_objid(i);

    }
    if (max_bullets < 100) {
        objid bullet = alloc_objid(O_bullet);
        struct entity *o = &obj[bullet];
        o->is_active = true;
        o->position = (Vector2){
            rand_between(s->sp.position.x, s->sp.position.x + 300.0f),
            rand_between(s->sp.position.y, s->sp.position.y + 300.0f),
        };
        o->lifetime = rand_between(60, 180);
    }
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
            sprite_set_position(&s->sp, (Vector2){s->position.x, s->position.y});
            sprite_set_rotation(&s->sp, s->rotation);
            sprite_update(&s->sp);
            break;
        case O_bullet:
            break;
        case O_enemy:
            break;
    }
}
