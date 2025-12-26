#include <stdio.h>
#include <raylib.h>
#include "game.h"
#include "spaceship.h"
#include "sprite.h"

#define ARRAY_COUNT(arr) (int32_t)(sizeof(arr) / sizeof(arr[0]))
#define UNUSED __attribute__((unused))

struct game_state g;
struct spaceship_state s;

int main(void)
{
    g = game_init();
    printf("fps = %d\n", g.fps);
    printf("display_width = %d\n", g.display_width);
    printf("display_height = %d\n", g.display_height);
    printf("current_scene = %d\n", g.current_scene);

    s = spaceship_init();
    printf("x = %d", s.x);
    printf("y = %d", s.y);
    printf("width = %d", s.width);
    printf("height = %d", s.height);
    printf("acce_x = %d", s.acce_x);
    printf("acce_y = %d", s.acce_y);

    InitWindow(g.display_width, g.display_height, "Comet");
    SetTargetFPS(g.fps);

    s.sp = sprite_init("graphics/spaceship/UFO.png", 1, (uint32_t[]){4});
    s.sp.scale = (Vector2){2.0f, 2.0f};

    while (!g.display_should_close)
    {
        game_update();
    }
    CloseWindow();

    return 0;
}

struct game_state game_init(void)
{
    return (struct game_state) {
        .fps = 60,
        .display_width = 1280,
        .display_height = 720,
        .current_scene = 1,
        .display_should_close = 0
    };
}

uint32_t game_update(void)
{
    BeginDrawing();
    ClearBackground(BLACK);

    g.display_should_close = WindowShouldClose();
    game_key_down();

    switch (g.current_scene) {
        case 1:
            DrawText("SCENE : MENU", 0, 0, 20, RAYWHITE);
            break;
        case 2:
            DrawText("Press 'M' to return to Menu", 0, 0, 20, RAYWHITE);
            break;
    }

    DrawRectanglePro(
            (Rectangle) {.x=s.x, .y=s.y, .width=s.width, .height=s.height}, 
            (Vector2){ 0.0f, 0.0f }, 
            s.rotation, 
            WHITE
            );

    sprite_set_position(&s.sp, (Vector2){s.x, s.y});
    sprite_update(&s.sp);

    EndDrawing();
    return 0;
}

uint32_t game_key_down(void)
{
    spaceship_set_action(&s, GetKeyPressed());
    switch (GetKeyPressed()) {
        case KEY_M:
            printf("open menu\n");
            g.current_scene = 1;
            break;
    }
    return 0;
}
