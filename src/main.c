#include <stdio.h>
#include <raylib.h>
#include "game.h"
#include "entity.h"
#include "camera.h"
#include <assert.h>

#define STRING_BUILDER_IMPLEMENTATION
#include "sb.h"

#include "pmem_usage.h"

#define ARRAY_COUNT(arr) (int32_t)(sizeof(arr) / sizeof(arr[0]))
#define UNUSED __attribute__((unused))

uint32_t rand_between(uint32_t min, uint32_t max) {
    return (uint32_t)(rand() % (max - min + 1)) + min;
}

struct console cmd;
struct entity *obj;
struct game_state gs;
struct camera_entity ce;
entity_id player = ENTITY_NONE;
RenderTexture2D screen;
int menu_option;

static void draw_background_grid(void)
{
    for (int i = 0; i <= WORLD_SIZE; i += GRID_SIZE) {
        DrawLine(i, 0, i, WORLD_SIZE, LIGHTGRAY);
        DrawLine(0, i, WORLD_SIZE, i, LIGHTGRAY);
    }
}

static void draw_menu(void)
{
    DrawText("RLCOMET", 138, 45, 12, RAYWHITE);
    DrawText(menu_option == 0 ? "> START" : "  START", 132, 85, 10, RAYWHITE);
    DrawText(menu_option == 1 ? "> EXIT"  : "  EXIT",  132, 105, 10, RAYWHITE);
}

int main(void)
{
    gs = game_init();

    InitWindow(gs.display_width, gs.display_height, "Comet");
    SetTargetFPS(gs.fps);

    screen = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(screen.texture, TEXTURE_FILTER_POINT);
    assets_load();

    ce = camera_entity_init((Vector2){VIRTUAL_WIDTH, VIRTUAL_HEIGHT});

    player = entity_alloc(O_player);
    assert(player != ENTITY_NONE);
    camera_entity_set_target_entity(&ce, player);

    struct entity *player_entity = entity_get(player);
    assert(player_entity);
    player_entity->sp = sprite_init(TEX_PLAYER, 1, (uint32_t[]){4});
    player_entity->sp.scale = (Vector2){2.0f, 2.0f};

    cmd = console_init();

    while (!gs.display_should_close)
    {
        game_update();
    }

    console_free(&cmd);
    arrfree(obj);
    assets_free();
    UnloadRenderTexture(screen);
    CloseWindow();
    return 0;
}

struct game_state game_init(void)
{
    struct game_state new = {0};
    new.fps = 60;
    new.display_width = 1280;
    new.display_height = 720;
    new.current_window = 1;
    new.display_should_close = 0;
    new.is_paused = false;
    return new;
}

uint32_t game_update(void)
{
    // TODO 202610031114 create a struct to store all flags
    static bool pmem_enabled = false;
    static bool editor_grid_enabled = false;
    static bool fps_enabled = false;

    gs.display_should_close = WindowShouldClose();
    game_input();

    if (gs.is_console_enabled) {
        console_update(&cmd);
        if (cmd.command_ready) {
            if (strcmp(cmd.command, "pmem") == 0) {
                pmem_enabled = !pmem_enabled;
            }
            else if (strcmp(cmd.command, "grid") == 0) {
                editor_grid_enabled = !editor_grid_enabled;
            }
            else if (strcmp(cmd.command, "collision") == 0) {
            // TODO 202609212101 add command to show all collisions
            // and invisible event blocks
            }
            else if (strcmp(cmd.command, "fps") == 0) {
                fps_enabled = !fps_enabled;
            }
            cmd.command[0] = '\0';
            cmd.command_ready = false;
            // TODO 202609192245 execute command line from console
            // TODO 202609201607 implement a argument parser
        }
    }

    BeginTextureMode(screen);
        ClearBackground(BLACK);

        switch (gs.current_window) {
            case 1:
                draw_menu();
                break;
            case 2:
                BeginMode2D(ce.camera);
                    if (!gs.is_paused) {
                        camera_entity_update(&ce);
                        // TODO 202610012008 enable background grid on console
                        if (editor_grid_enabled) {
                            draw_background_grid();
                        }
                        for (ptrdiff_t i = 0; i < arrlen(obj); ++i) {
                            entity_update(&obj[i]);
                        }
                    }
                EndMode2D();
                DrawText("B: MENU", 4, 4, 8, RAYWHITE);
                break;
            default:
                //nothing
        }

    EndTextureMode();

    BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(screen.texture,
            (Rectangle){0, 0, VIRTUAL_WIDTH, -VIRTUAL_HEIGHT},
            (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
            (Vector2){0}, 0, WHITE);
        if (gs.is_console_enabled) {
            console_draw(&cmd);
        }
        if (pmem_enabled) {
            char text[164];
            uint64_t bytes = pmem_usage();
            double megabytes = (double)bytes / 1024.0 / 1024.0;
            snprintf(text, sizeof(text), "Memory usage: %.2f MB", megabytes);
            DrawText(text, 40, 40, 24, RED);
        }
        if (fps_enabled) {
            DrawFPS(50, 50);
        }
    EndDrawing();
    return 0;
}

uint32_t game_input(void)
{
    int key = GetKeyPressed();

    // TODO 202609212125 always execute console keyboard logic first
    switch (key) {
        case KEY_GRAVE:
            gs.is_console_enabled = !gs.is_console_enabled;
            //gs.is_paused = !gs.is_paused;
            break;
    }

    if (gs.current_window == 1 && !gs.is_console_enabled) {
        if (key == KEY_UP || key == KEY_W) menu_option = 0;
        if (key == KEY_DOWN || key == KEY_S) menu_option = 1;
        if (key == KEY_ENTER) {
            if (menu_option == 0) gs.current_window = 2;
            else gs.display_should_close = 1;
        }
        return 0;
    }

    if (!gs.is_paused && !gs.is_console_enabled) {
        struct entity *player_entity = entity_get(player);
        if (player_entity) {
            entity_set_action(player_entity, key);
        }
    }

    // TODO 202609212121 disable others input when console is open
    if (!gs.is_console_enabled) {
        switch (key) {
            case KEY_P:
                console_text_append(&cmd.trace, LT_INFO, sb_stringf("Game was PAUSED"));
                gs.is_paused = !gs.is_paused;
                break;
            case KEY_M: {
                console_text_append(&cmd.trace, LT_ERROR, sb_stringf("open menu %td", arrlen(cmd.trace)));
                camera_entity_trigger_camera_shake(&ce, 1.0f, 300.0f);
                struct entity *player_entity = entity_get(player);
                if (player_entity) {
                    player_entity->speed = (Vector2) {0,0};
                }
                break;
            }
            case KEY_B: {
                gs.current_window = 1;
                break;
            }
        }
    }
    return 0;
}
