#include "spaceship.h"
#include <raylib.h>
#include <stdio.h>

struct spaceship_state spaceship_init(void)
{
    return (struct spaceship_state) {
        .x = 0, 
        .y = 0,
        .width = 32,
        .height = 32,
        .acce_x = 1, 
        .acce_y = 0,
        .rotation = 0.0f,
    };
}

uint32_t spaceship_set_action(struct spaceship_state *s, uint32_t action)
{
    s->x += s->acce_x;
    s->y += s->acce_y;
    s->rotation += 1;
    switch (action) {
        case KEY_SPACE:
            printf("shoot missle\n");
            spaceship_act_shoot_missle(s);
            break;
        case KEY_UP:
            printf("increase acceleration\n");
            spaceship_act_move_up(s);
            break;
        case KEY_DOWN:
            printf("decrease acceleration\n");
            spaceship_act_move_down(s);
            break;
        case KEY_LEFT:
            printf("rotate the spaceship to left\n");
            spaceship_act_move_left(s);
            break;
        case KEY_RIGHT:
            printf("rotate the spaceship to right\n");
            spaceship_act_move_right(s);
            break;
    }
    return 0;
}

uint32_t spaceship_act_move_up(struct spaceship_state *s)
{
    if (s->acce_x > -2)
        if (s->acce_y > -2)
            s->acce_y -= 1;
    return 0;
}

uint32_t spaceship_act_move_down(struct spaceship_state *s)
{
    if (s->acce_x > -2)
        if (s->acce_y < 2)
            s->acce_y += 1;
    return 0;
}

uint32_t spaceship_act_shoot_missle(struct spaceship_state *s)
{
    return 0;
}

uint32_t spaceship_act_move_left(struct spaceship_state *s)
{
    if (s->acce_x > -2)
        s->acce_x -= 1;
    return 0;
}

uint32_t spaceship_act_move_right(struct spaceship_state *s)
{
    if (s->acce_x < 2)
        s->acce_x += 1;
    return 0;
}
