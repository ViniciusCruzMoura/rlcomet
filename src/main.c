#include <stdio.h>
#include <raylib.h>
#include "game.h"
#include "entity.h"
#include "camera.h"
#include "level_editor.h"
#include <string.h>
#include "sb.h"
#include "pmem_usage.h"

uint32_t rand_between(uint32_t min, uint32_t max) {
    return (uint32_t)(rand() % (max - min + 1)) + min;
}

struct console cmd;
struct game_state gs;
RenderTexture2D screen;
int menu_option;
struct level_editor editor = {.grid = true, .selected = -1};

// TODO 202610031114 create a struct to store all flags
static bool pmem_enabled = false;
static bool fps_enabled = false;

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
    if (!DirectoryExists(ASSETS_DIR)) {
        MakeDirectory(ASSETS_DIR);
    }

    screen = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(screen.texture, TEXTURE_FILTER_POINT);
    gs.ce = camera_entity_init((Vector2){VIRTUAL_WIDTH, VIRTUAL_HEIGHT});
    editor.camera = gs.ce.camera;
    editor.camera.target = editor.camera.offset;
    cmd = console_init();
    if (!level_load(&gs.levels, ASSETS_FILE)) {
        console_text_append(&cmd.trace, LT_ERROR, sb_stringf("ERROR: cannot load %s (%s)",
            ASSETS_FILE, FileExists(ASSETS_FILE) ? "invalid file" : "file not found"));
        gs.is_console_enabled = true;
        struct level map;
        level_init(&map, WORLD_SIZE / GRID_SIZE, WORLD_SIZE / GRID_SIZE, GRID_SIZE);
        level_add_layer(&map, LEVEL_BACKGROUND);
        level_add_layer(&map, LEVEL_GROUND);
        level_add_layer(&map, LEVEL_FOREGROUND);
        arrput(gs.levels, map);
    }
    assets_load();
    editor.levels = &gs.levels;
    editor.current_level = &gs.current_level;

    struct entity e = entity_init(PLAYER);
    e.unique = true;
    e.collider = physics_polygon_rectangle((Rectangle){0, 0, GRID_SIZE, GRID_SIZE});
    e.speed = (Vector2){0, 0};
    e.sp = sprite_init(TEX_PLAYER, PLAYER_ANIMATION_COUNT, (uint32_t[]){5, 5, 5, 5, 5, 5}, 6);
    arrput(editor.templates, e);

    e = entity_init(ENEMY);
    e.collider = physics_polygon_rectangle((Rectangle){0, 0, GRID_SIZE, GRID_SIZE});
    e.speed = (Vector2){0, 0};
    e.sp = sprite_init(TEX_PLAYER, PLAYER_ANIMATION_COUNT, (uint32_t[]){5, 5, 5, 5, 5, 5}, 6);
    e.sp.tint = RED;
    arrput(editor.templates, e);

    e = entity_bullet_init();
    e.speed = (Vector2){0, 0};
    arrput(editor.templates, e);

    uint32_t tile_texture = assets_texture(TEX_TILESET).id ? TEX_TILESET : TEX_NO_TEXTURE;
    Texture2D tileset = assets_texture(tile_texture);
    uint32_t rows = tileset.height / GRID_SIZE ? tileset.height / GRID_SIZE : 1;
    uint32_t columns = tileset.width / GRID_SIZE ? tileset.width / GRID_SIZE : 1;
    for (uint32_t y = 0; y < rows; ++y) {
        for (uint32_t x = 0; x < columns; ++x) {
            e = entity_init(NONE);
            e.speed = (Vector2){0, 0};
            e.collider = physics_polygon_rectangle((Rectangle){0, 0, GRID_SIZE, GRID_SIZE});
            e.sp.texture_id = tile_texture;
            e.sp.tint = WHITE;
            e.sp.scale = (Vector2){1, 1};
            e.sp.frame_rec = (Rectangle){x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE};
            arrput(editor.templates, e);
        }
    }

    while (!gs.display_should_close)
    {
        game_update();
        game_draw();
    }

    console_free(&cmd);
    for (int i = 0; i < arrlen(gs.levels); ++i) {
        level_free(&gs.levels[i]);
    }
    arrfree(gs.levels);
    for (int i = 0; i < arrlen(editor.templates); ++i) {
        entity_free(&editor.templates[i]);
    }
    arrfree(editor.templates);
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
    new.current_window = WINDOW_MAIN_MENU;
    new.display_should_close = 0;
    new.is_paused = false;
    return new;
}

uint32_t game_update(void)
{
    // TODO 202610032054 dont use ESC to close game, 
    // ESC will be used to show menu
    gs.display_should_close = WindowShouldClose();
    game_input();

    if (gs.is_console_enabled) {
        console_update(&cmd);
        if (cmd.command_ready) {
            if (strcmp(cmd.command, "pmem") == 0) {
                pmem_enabled = !pmem_enabled;
            }
            else if (strcmp(cmd.command, "collision") == 0) {
                // TODO 202609212101 add command to show all collisions
                // and invisible event blocks
            }
            else if (strcmp(cmd.command, "fps") == 0) {
                fps_enabled = !fps_enabled;
            }
            else if (strcmp(cmd.command, "save") == 0) {
                if (!DirectoryExists(ASSETS_DIR)) {
                    MakeDirectory(ASSETS_DIR);
                }
                bool saved = level_save(gs.levels, ASSETS_FILE);
                console_text_append(&cmd.trace, saved ? LT_INFO : LT_ERROR,
                    sb_stringf("%s %s", saved ? "Level saved:" : "ERROR: cannot save", ASSETS_FILE));
            }
            else if (strcmp(cmd.command, "load") == 0) {
                if (!level_load(&gs.levels, ASSETS_FILE)) {
                    console_text_append(&cmd.trace, LT_ERROR, sb_stringf("ERROR: cannot load %s", ASSETS_FILE));
                } else {
                    assets_load();
                    gs.current_level = 0;
                    editor.selected = -1;
                    console_text_append(&cmd.trace, LT_INFO, sb_stringf("Level loaded: %s", ASSETS_FILE));
                }
            }
            else if (strncmp(cmd.command, "gfx ", 4) == 0) {
                uint32_t id;
                char path[256];
                if (sscanf(cmd.command + 4, "%u %255s", &id, path) != 2 || !assets_import(id, path)) {
                    console_text_append(&cmd.trace, LT_ERROR, sb_stringf("ERROR: cannot import graphic: %s", cmd.command));
                }
            }
            else if (strcmp(cmd.command, "newlevel") == 0) {
                struct level next;
                if (arrlen(gs.levels) < LEVEL_MAX_LEVELS &&
                    level_init(&next, WORLD_SIZE / GRID_SIZE, WORLD_SIZE / GRID_SIZE, GRID_SIZE)) {
                    level_add_layer(&next, LEVEL_BACKGROUND);
                    level_add_layer(&next, LEVEL_GROUND);
                    level_add_layer(&next, LEVEL_FOREGROUND);
                    arrput(gs.levels, next);
                    gs.current_level = arrlen(gs.levels) - 1;
                    editor.selected = -1;
                }
            }
            else if (strcmp(cmd.command, "editor") == 0) {
                gs.current_window = WINDOW_GAMEPLAY;
                editor.enabled = true;
                gs.is_console_enabled = false;
                cmd.input_active = false;
            }
            cmd.command[0] = '\0';
            cmd.command_ready = false;
            // TODO 202609192245 execute command line from console
            // TODO 202609201607 implement a argument parser
        }
    }

    switch (gs.current_window) {
        case WINDOW_GAMEPLAY:
            if (!gs.is_paused && !editor.enabled && !gs.is_console_enabled) {
                struct level *map = &gs.levels[gs.current_level];
                for (ptrdiff_t i = 0; i < arrlen(map->entities); ++i) {
                    entity_update(&map->entities[i]);
                    if (!map->entities[i].is_active) {
                        arrdel(map->entities, i);
                        --i;
                    }
                }
                camera_entity_update(&gs.ce, map->entities);
            }
            break;
        default:
            break;
    }
    return 0;
}

void game_draw(void)
{
    BeginTextureMode(screen);
        ClearBackground(BLACK);

        switch (gs.current_window) {
            case WINDOW_MAIN_MENU:
                draw_menu();
                break;
            case WINDOW_GAMEPLAY:
                // TODO 202610012008 enable background grid on console
                level_editor_draw(&editor, editor.enabled ? editor.camera : gs.ce.camera);
                if (!editor.enabled) {
                    DrawText("B: MENU", 4, 4, 8, RAYWHITE);
                }
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
        if (gs.current_window == WINDOW_GAMEPLAY && !gs.is_console_enabled) {
            level_editor_gui(&editor);
        }
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

    switch (gs.current_window) {
        case WINDOW_MAIN_MENU:
            if (!gs.is_console_enabled) {
                if (key == KEY_UP || key == KEY_W) {
                    menu_option = 0;
                }
                if (key == KEY_DOWN || key == KEY_S) {
                    menu_option = 1;
                }
                if (key == KEY_ENTER) {
                    if (menu_option == 0) {
                        gs.current_window = WINDOW_GAMEPLAY;
                    } else {
                        gs.display_should_close = 1;
                    }
                }
            }
            return 0;
        case WINDOW_GAMEPLAY:
            if (!gs.is_console_enabled) {
                level_editor_update(&editor,
                                    (Vector2){TO_VIRTUAL_X(GetMouseX()), TO_VIRTUAL_Y(GetMouseY())},
                                    (Vector2){VIRTUAL_WIDTH, VIRTUAL_HEIGHT});
                if (editor.saved == 1) {
                    console_text_append(&cmd.trace, LT_INFO, sb_stringf("Level saved: %s", ASSETS_FILE));
                    editor.saved = 0;
                } else if (editor.saved == 2) {
                    assets_load();
                    console_text_append(&cmd.trace, LT_INFO, sb_stringf("Level loaded: %s", ASSETS_FILE));
                    editor.saved = 0;
                } else if (editor.saved < 0) {
                    console_text_append(&cmd.trace, LT_ERROR,
                        sb_stringf("ERROR: cannot %s %s", editor.saved == -1 ? "save" : "load", ASSETS_FILE));
                    gs.is_console_enabled = true;
                    editor.saved = 0;
                }
            }
            break;
        default:
            return 0;
    }

    // TODO 202609212121 disable others input when console is open
    if (!gs.is_console_enabled) {
        switch (key) {
            case KEY_P:
                console_text_append(&cmd.trace, LT_INFO, sb_stringf("Game was PAUSED"));
                gs.is_paused = !gs.is_paused;
                break;
            case KEY_B: {
                gs.current_window = WINDOW_MAIN_MENU;
                break;
            }
        }
    }
    return 0;
}
