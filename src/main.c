#include <stdio.h>
#include <raylib.h>
#include "game.h"
#include "entity.h"
#include "camera.h"

#define ARRAY_COUNT(arr) (int32_t)(sizeof(arr) / sizeof(arr[0]))
#define UNUSED __attribute__((unused))

struct game_state g;
struct entity s;
struct camera_entity c;

int main(void)
{
    g = game_init();
    c = camera_entity_init((Vector2){g.display_width, g.display_height});
    s = entity_init();
    camera_entity_set_target_entity(&c, &s);

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
        .display_should_close = 0,
        .is_paused = false,
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

        if (!g.is_paused) {
            BeginMode2D(c.camera);
                camera_entity_update(&c);
                entity_update(&s);
            EndMode2D();
        }

    EndDrawing();
    return 0;
}

uint32_t game_key_down(void)
{
    entity_set_action(&s, GetKeyPressed());
    switch (GetKeyPressed()) {
        case KEY_M:
            printf("open menu\n");
            camera_entity_trigger_camera_shake(&c, 1.0f, 300.0f);
            s.speed = (Vector2) {0,0};
            g.current_scene = 1;
            break;
    }
    return 0;
}
