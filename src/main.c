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

char* stringf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int size = vsnprintf(NULL, 0, format, args);
    if (size < 0) {
        va_end(args);
        return NULL;
    }
    size++;
    char* result = malloc(size);
    if (!result) {
        va_end(args);
        return NULL;
    }
    va_end(args);

    va_start(args, format);
    vsnprintf(result, size, format, args);
    va_end(args);
    return result;
}

typedef enum {
    LT_ERROR,
    LT_INFO,
    LT_WARNING,
} LogTraceLevel;

typedef struct LogTrace {
    int level;
    char* message;
} LogTrace;

typedef struct Vector {
    void** itens;
    int used;
    int capacity;
} Vector;

void append(Vector* v, void* item) {
    if (v->used >= v->capacity) {
        v->capacity += 256;
        v->itens = realloc(v->itens, v->capacity*sizeof(v->itens));
    }
    v->itens[v->used] = item;
    ++v->used;
}

void* at(Vector* v, int i) {
    if (i <= v->used && i >= 0) {
        return v->itens[i];
    }
    return NULL;
}

void remove_at(Vector* v, int idx) {
    if (idx <= v->used && idx >=0) {
        for(int i = idx; i < v->used; ++i) {
            v->itens[i] = v->itens[i+1];
        }
        --v->used;
    }
}

void traceback(void* trace, int level, char* msg) {
    LogTrace* log = malloc(sizeof(LogTrace));
    log->level = level;
    log->message = msg;
    append(trace, log);
}

void DrawTraceback(Vector* trace) {
    if (trace != NULL) {
        for(int i=0; i<trace->used; ++i) {
            LogTrace* log = at(trace, i);
            if (log != NULL) {
                DrawText(log->message, 1, 20*(i+1), 20, WHITE);
            }
            if (i>30) remove_at(trace, i);
        }
    }
}

Vector trace;

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
                entity_update(&s);
            EndMode2D();
        }

        DrawTraceback(&trace);
    EndDrawing();
    return 0;
}

uint32_t game_key_down(void)
{
    int key = GetKeyPressed();
    entity_set_action(&s, key);
    switch (key) {
        case KEY_M:
            traceback(&trace, LT_INFO, stringf("open menu %d", trace.used));
            camera_entity_trigger_camera_shake(&c, 1.0f, 300.0f);
            s.speed = (Vector2) {0,0};
            g.current_scene = 1;
            break;
    }
    return 0;
}
