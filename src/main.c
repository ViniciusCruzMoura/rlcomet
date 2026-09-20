#include <stdio.h>
#include <raylib.h>
#include "game.h"
#include "entity.h"
#include "camera.h"
#include <assert.h>

#define STRING_BUILDER_IMPLEMENTATION
#include "sb.h"

#define ARRAY_COUNT(arr) (int32_t)(sizeof(arr) / sizeof(arr[0]))
#define UNUSED __attribute__((unused))

uint32_t rand_between(uint32_t min, uint32_t max) {
    return (uint32_t)(rand() % (max - min + 1)) + min;
}

struct console cmd;

uint32_t max_obj;
struct entity obj[MAX_OBJECTS];
struct game_state g;
struct camera_entity c;
objid player;

objid alloc_objid(uint32_t type)
{
    uint32_t i;
    for (i=1; i < MAX_OBJECTS; ++i) {
        if (!obj[i].is_active || obj[i].type == O_none) {
            memset(&obj[i], 0, sizeof(obj[0]));

            obj[i].is_active = 1;
            obj[i].type = type;

            if (i > max_obj) max_obj = i;
            return i;
        }
    }
    assert(0);
    return 0;
}

void free_objid(objid id)
{
    obj[id].is_active = 0;
    obj[id].type = O_none;
}

void draw_background_grid(void)
{
    uint32_t square_size = 40;
    for (int i = 0; i < g.display_width/square_size + 1; i++)
    {
        DrawLineV((Vector2){(float)square_size*i, 0}, (Vector2){ (float)square_size*i, (float)g.display_height}, LIGHTGRAY);
    }
    for (int i = 0; i < g.display_height /square_size + 1; i++)
    {
        DrawLineV((Vector2){0, (float)square_size*i}, (Vector2){ (float)g.display_width, (float)square_size*i}, LIGHTGRAY);
    }
    for (int i = 0; i < g.display_width/square_size; i++)
    {
        for (int j = 0; j < g.display_height/square_size; j++)
        {
            DrawText(TextFormat("[%i,%i]", i, j), 10 + square_size*i, 15 + square_size*j, 10, LIGHTGRAY);
        }
    }
}

int main(void)
{
    g = game_init();
    
    InitWindow(g.display_width, g.display_height, "Comet");
    SetTargetFPS(g.fps);
    
    c = camera_entity_init((Vector2){g.display_width, g.display_height});
    
    player = alloc_objid(O_player);
    obj[player] = entity_init(O_player);
    struct entity *pobj = &obj[player];
    camera_entity_set_target_entity(&c, pobj);
    pobj->sp = sprite_init("graphics/spaceship/UFO.png", 1, (uint32_t[]){4});
    pobj->sp.scale = (Vector2){2.0f, 2.0f};

    // TODO 202609201421 make a contructor function?
    // console area need be dynamic so if the screen
    // change, the size need change too
    cmd = (struct console) {
        .area = (Rectangle){.x=1, .y=20, .width=GetScreenWidth()/2, .height=GetScreenHeight()/2},
        .input_area = {1, GetScreenHeight()/2 + 20, GetScreenWidth()/2, 24},
        .scrollbar = {GetScreenWidth()/2, 20, 10, GetScreenHeight()/2},
        .rows = 20,
        .row_height = 20,
        .last_used = -1
    };

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
                for (uint32_t i = 1; i <= max_obj; ++i) {
                    if (obj[i].type == O_none) continue;
                    entity_update(&obj[i]);
                }
                if (g.is_console_enabled) {
                    draw_background_grid();
                }
            EndMode2D();
        }

        if (g.is_console_enabled) {
            console_update(&cmd);
            if (cmd.command_ready) {
                //TODO 202609192245 execute command line from console
                //TODO 202609201607 implement a argument parser
            }
        }
        if (g.is_console_enabled) {
            console_draw(&cmd);
        }
    EndDrawing();
    return 0;
}

uint32_t game_key_down(void)
{
    struct entity *pobj = &obj[player];
    int key = GetKeyPressed();
    if (!g.is_paused) entity_set_action(pobj, key);
    switch (key) {
        case KEY_P:
            console_text_append(&cmd.trace, LT_INFO, sb_stringf("Game was PAUSED"));
            g.is_paused = !g.is_paused;
            break;
        case KEY_GRAVE:
            console_text_append(&cmd.trace, LT_WARNING, sb_stringf("Enabled the console log"));
            g.is_console_enabled = !g.is_console_enabled;
            break;
        case KEY_M:
            console_text_append(&cmd.trace, LT_ERROR, sb_stringf("open menu %d", cmd.trace.used));
            camera_entity_trigger_camera_shake(&c, 1.0f, 300.0f);
            pobj->speed = (Vector2) {0,0};
            g.current_scene = 1;
            break;
    }
    return 0;
}

