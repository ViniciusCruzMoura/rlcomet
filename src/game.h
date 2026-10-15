#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "level.h"
#include "camera.h"
#include "console.h"
#include "stb_ds.h"
#include "assets.h"

#define VIRTUAL_WIDTH 320
#define VIRTUAL_HEIGHT 180
#define WORLD_SIZE 1024
#define GRID_SIZE 16
#define TO_VIRTUAL_X(x) ((float)(x) * VIRTUAL_WIDTH / GetScreenWidth())
#define TO_VIRTUAL_Y(y) ((float)(y) * VIRTUAL_HEIGHT / GetScreenHeight())

extern struct game_state gs;
extern RenderTexture2D screen;

extern struct console cmd;

enum {
    WINDOW_MAIN_MENU = 1,
    WINDOW_GAMEPLAY
};

struct game_state {
    struct camera_entity ce;
    struct level *levels;
    uint32_t current_level;
    uint32_t fps;
    uint32_t display_width;
    uint32_t display_height;
    uint32_t current_window;
    uint32_t display_should_close;
    bool is_paused;
    bool is_console_enabled;
    // TODO 202609232330 add pmem enabled
    // TODO 202609232331 add grid enabled
};

struct game_state game_init(void);
uint32_t game_input(void);
uint32_t game_update(void);
// TODO 202610011947 separate update from draw
// void game_draw(struct game_state *instance);
// void game_free(struct game_state *instance);
void game_draw(void);

uint32_t rand_between(uint32_t min, uint32_t max);

#endif //GAME_H
