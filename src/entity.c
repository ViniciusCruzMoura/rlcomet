#include "entity.h"
#include "sprite.h"
#include <raylib.h>
#include <stdio.h>

struct entity entity_init(void)
{
    return (struct entity) {
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
    switch (action) {
        case KEY_SPACE:
            printf("shoot missle\n");
            entity_act_shoot_missle(s);
            break;
        case KEY_UP:
            printf("increase acceleration\n");
            entity_act_move_up(s);
            break;
        case KEY_DOWN:
            printf("decrease acceleration\n");
            entity_act_move_down(s);
            break;
        case KEY_LEFT:
            printf("rotate the spaceship to left\n");
            entity_act_move_left(s);
            break;
        case KEY_RIGHT:
            printf("rotate the spaceship to right\n");
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
    return 0;
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
    sprite_set_position(&s->sp, (Vector2){s->position.x, s->position.y});
    sprite_set_rotation(&s->sp, s->rotation);
    sprite_update(&s->sp);
}
