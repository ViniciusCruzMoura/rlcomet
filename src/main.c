#include <stdio.h>
#include <raylib.h>
#include "game.h"
#include "entity.h"
#include "camera.h"
#include <assert.h>

#define STRING_BUILDER_IMPLEMENTATION
#include "sb.h"

#define PMEM_USAGE_IMPLEMENTATION
#include "pmem_usage.h"

#define ARRAY_COUNT(arr) (int32_t)(sizeof(arr) / sizeof(arr[0]))
#define UNUSED __attribute__((unused))

uint32_t rand_between(uint32_t min, uint32_t max) {
    return (uint32_t)(rand() % (max - min + 1)) + min;
}

struct console cmd;
struct darray obj;
struct game_state g;
struct camera_entity c;
struct entity *player;

// TODO 202609212028 rename to entity_alloc(uint32_t type)
struct entity *alloc_obj(uint32_t type)
{
    struct entity *o;
    for (int i = 0; i < obj.used; ++i) {
        o = darray_at(&obj, i);
        if (!o->is_active || o->type == O_none) {
            *o = entity_init(type);
            return o;
        }
    }

    o = malloc(sizeof *o);
    if (!o) return NULL;
    *o = entity_init(type);
    if (!darray_append(&obj, o)) {
        free(o);
        return NULL;
    }
    return o;
}

void free_obj(struct entity *o)
{
    o->is_active = 0;
    o->type = O_none;
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
    
    player = alloc_obj(O_player);
    assert(player);
    camera_entity_set_target_entity(&c, player);
    // TODO 202609212036 create a assets manager
    player->sp = sprite_init("graphics/spaceship/UFO.png", 1, (uint32_t[]){4});
    player->sp.scale = (Vector2){2.0f, 2.0f};

    // TODO 202609201421 make a contructor function?
    // console area need be dynamic so if the screen
    // change, the size need change too
    cmd = (struct console) {
        .area = (Rectangle){.x=1, .y=20, .width=GetScreenWidth()/2, .height=GetScreenHeight()/2},
        .input_area = {1, GetScreenHeight()/2 + 20, GetScreenWidth()/2, 24},
        .scrollbar = {GetScreenWidth()/2, 20, 10, GetScreenHeight()/2},
        .rows = 18,
        .row_height = 20,
        .last_used = -1
    };

    while (!g.display_should_close)
    {
        game_update();
    }
    
    CloseWindow();

    for (int i = 0; i < obj.used; ++i) free(darray_at(&obj, i));
    darray_free(&obj);
    return 0;
}

struct game_state game_init(void)
{
    return (struct game_state) {
        .fps = 60,
        .display_width = 1280,
        .display_height = 720,
        .current_window = 1,
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

        switch (g.current_window) {
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
                for (int i = 0; i < obj.used; ++i) {
                    entity_update(darray_at(&obj, i));
                }
                if (g.is_console_enabled) {
                    draw_background_grid();
                }
            EndMode2D();
        }

        if (g.is_console_enabled) {
            console_update(&cmd);
            if (cmd.command_ready) {
                // TODO 202609192245 execute command line from console
                // TODO 202609201607 implement a argument parser
                // TODO 202609212058 add a fps_enabled command 
                // to show fps and pmem usage
                // TODO 202609212101 add command to show all collisions
                // and invisible event blocks
            }
        }
        if (g.is_console_enabled) {
            console_draw(&cmd);
        }

        if (0) {
            char text[164];
            uint64_t bytes = pmem_usage();
            double megabytes = (double)bytes / 1024.0 / 1024.0;
            snprintf(text, sizeof(text), "Memory usage: %.2f MB", megabytes);
            DrawText(text, 40, 40, 24, RED);
        }
    EndDrawing();
    return 0;
}

uint32_t game_key_down(void)
{
    int key = GetKeyPressed();

    // TODO 202609212125 always execute console keyboard logic first
    switch (key) {
        case KEY_GRAVE:
            g.is_console_enabled = !g.is_console_enabled;
            //g.is_paused = !g.is_paused;
            break;
    }

    if (!g.is_paused && !g.is_console_enabled) {
        entity_set_action(player, key);
    }

    // TODO 202609212121 disable others input when console is open
    if (!g.is_console_enabled) {
        switch (key) {
            case KEY_P:
                console_text_append(&cmd.trace, LT_INFO, sb_stringf("Game was PAUSED"));
                g.is_paused = !g.is_paused;
                break;
            case KEY_M:
                console_text_append(&cmd.trace, LT_ERROR, sb_stringf("open menu %d", cmd.trace.used));
                camera_entity_trigger_camera_shake(&c, 1.0f, 300.0f);
                player->speed = (Vector2) {0,0};
                g.current_window = 1;
                break;
        }
    }
    return 0;
}
