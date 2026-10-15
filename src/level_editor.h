#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include <raylib.h>
#include "level.h"

struct level_editor {
    Camera2D camera;
    bool enabled;
    bool grid;
    uint32_t layer;
    uint32_t palette_start;
    int brush;
    int selected;
    int saved;
    struct entity *templates;
    struct level **levels;
    uint32_t *current_level;
};

void level_editor_update(struct level_editor *editor, Vector2 mouse, Vector2 viewport);
void level_editor_draw(const struct level_editor *editor, Camera2D camera);
void level_editor_gui(struct level_editor *editor);

#endif
