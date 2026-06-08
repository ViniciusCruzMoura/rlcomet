#include <stdio.h>
#include <raylib.h>
#include "game.h"
#include "entity.h"
#include "camera.h"
#include "common.h"
#include <assert.h>
#include <string.h>

#define ARRAY_COUNT(arr) (int32_t)(sizeof(arr) / sizeof(arr[0]))
#define UNUSED __attribute__((unused))

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

enum {
    LT_ERROR,
    LT_INFO,
    LT_WARNING,
};

typedef struct LogTrace {
    int level;
    char* message;
} LogTrace;

typedef struct Vector {
    void** itens;
    int used;
    int capacity;
} Vector;

void append(Vector* v, void* item)
{
    if (v->used >= v->capacity) {
        v->capacity += 256;
        v->itens = realloc(v->itens, v->capacity*sizeof(v->itens));
    }
    v->itens[v->used] = item;
    ++v->used;
}

void* at(Vector* v, int i)
{
    if (i <= v->used && i >= 0) {
        return v->itens[i];
    }
    return NULL;
}

void remove_at(Vector* v, int idx)
{
    if (idx <= v->used && idx >=0) {
        for(int i = idx; i < v->used; ++i) {
            v->itens[i] = v->itens[i+1];
        }
        --v->used;
    }
}

void traceback(void* trace, int level, char* msg)
{
    LogTrace* log = malloc(sizeof(LogTrace));
    log->level = level;
    log->message = msg;
    append(trace, log);
}

void draw_traceback(Vector* trace)
{
    if (trace != NULL) {
        for(int i=0; i<trace->used; ++i) {
            LogTrace* log = at(trace, i);
            if (log != NULL) {
                switch(log->level) {
                    case LT_INFO:
                        DrawText(log->message, 1, 20*(i+1), 20, WHITE);
                        break;
                    case LT_WARNING:
                        DrawText(log->message, 1, 20*(i+1), 20, YELLOW);
                        break;
                    case LT_ERROR:
                        DrawText(log->message, 1, 20*(i+1), 20, RED);
                        break;
                }
            }
//             if (i>30) remove_at(trace, i);
        }
    }
}

Vector trace;

void draw_background_grade(void)
{
    if (g.enable_traceback) {
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

    trace = (Vector) { .used=0, .capacity=0 };

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
                draw_background_grade();
            EndMode2D();
        }
        if (g.enable_traceback) {
            draw_traceback(&trace);
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
            traceback(&trace, LT_INFO, stringf("Game was PAUSED"));
            g.is_paused = !g.is_paused;
            break;
        case KEY_GRAVE:
            traceback(&trace, LT_WARNING, stringf("Enabled the traceback log"));
            g.enable_traceback = !g.enable_traceback;
            break;
        case KEY_M:
            traceback(&trace, LT_ERROR, stringf("open menu %d", trace.used));
            camera_entity_trigger_camera_shake(&c, 1.0f, 300.0f);
            pobj->speed = (Vector2) {0,0};
            g.current_scene = 1;
            break;
    }
    return 0;
}
