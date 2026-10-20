#include "level_editor.h"
#include "assets.h"
#include "game.h"
#include "stb_ds.h"
#include "raygui.h"
#include <math.h>
#include <limits.h>

#define EDITOR_ZOOM_MIN 0.25f
#define EDITOR_ZOOM_MAX 4.0f
#define EDITOR_ZOOM_STEP 0.25f
#define EDITOR_PAN_SPEED 160.0f
#define EDITOR_PANEL_WIDTH 250.0f
#define EDITOR_TEMPLATE_CELL 36.0f
#define EDITOR_INSPECTOR_HEIGHT 232.0f
#define EDITOR_INACTIVE_OPACITY 0.4f

static Rectangle entity_bounds(const struct entity *e, uint32_t tile_size)
{
    float w = fabsf(e->sp.frame_rec.width * e->sp.scale.x);
    float h = fabsf(e->sp.frame_rec.height * e->sp.scale.y);
    if (w <= 0) {
        w = tile_size;
    }
    if (h <= 0) {
        h = tile_size;
    }
    return (Rectangle){e->position.x, e->position.y, w, h};
}

static void editor_preview(struct sprite sp, Rectangle box)
{
    Texture2D tex = assets_texture(sp.texture_id);
    if (!tex.id || !sp.frame_rec.width || !sp.frame_rec.height) {
        tex = assets_texture(TEX_NO_TEXTURE);
        if (!tex.id) {
            DrawRectangleRec(box, RED);
            return;
        }
        sp.texture_id = TEX_NO_TEXTURE;
        sp.frame_rec = (Rectangle){0, 0, tex.width, tex.height};
        sp.tint = WHITE;
    } else if (sp.total_rows && sp.frames_per_row) {
        sp.frame_rec = (Rectangle){0, 0, (float)tex.width / sp.frames_per_row[0],
                                   (float)tex.height / sp.total_rows};
    }
    float scale = fminf(1, fminf(box.width / fabsf(sp.frame_rec.width),
                                box.height / fabsf(sp.frame_rec.height)));
    sp.scale = (Vector2){scale, scale};
    sprite_draw(&sp, (Vector2){box.x + (box.width - fabsf(sp.frame_rec.width * scale)) / 2,
                               box.y + (box.height - fabsf(sp.frame_rec.height * scale)) / 2});
}

void level_editor_update(struct level_editor *editor, Vector2 mouse, Vector2 viewport)
{
    if (IsKeyPressed(KEY_F1)) {
        editor->enabled = !editor->enabled;
        editor->selected = -1;
    }
    if (!editor->enabled || !editor->levels || !arrlen(*editor->levels)) {
        return;
    }
    float pan = EDITOR_PAN_SPEED * GetFrameTime() / editor->camera.zoom;
    if (!IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            editor->camera.target.x -= pan;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            editor->camera.target.x += pan;
        }
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
            editor->camera.target.y -= pan;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
            editor->camera.target.y += pan;
        }
    }
    if (*editor->current_level >= arrlenu(*editor->levels)) {
        *editor->current_level = 0;
    }
    struct level *level = &(*editor->levels)[*editor->current_level];
    if (!arrlen(level->layers)) {
        return;
    }
    if (editor->layer >= arrlenu(level->layers)) {
        editor->layer = 0;
    }
    if (IsKeyPressed(KEY_TAB)) {
        editor->layer = (editor->layer + 1) % arrlen(level->layers);
    }
    if (IsKeyPressed(KEY_G)) {
        editor->grid = !editor->grid;
    }
    if (IsKeyPressed(KEY_V)) {
        level->layers[editor->layer].visible = !level->layers[editor->layer].visible;
        editor->saved = 0;
    }
    if (IsKeyPressed(KEY_PAGE_DOWN) && *editor->current_level + 1 < arrlenu(*editor->levels)) {
        ++*editor->current_level;
        editor->selected = -1;
        return;
    }
    if (IsKeyPressed(KEY_PAGE_UP) && *editor->current_level > 0) {
        --*editor->current_level;
        editor->selected = -1;
        return;
    }
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_S)) {
            editor->saved = level_save(*editor->levels, ASSETS_FILE) ? 1 : -1;
        }
        if (IsKeyPressed(KEY_O)) {
            editor->saved = level_load(editor->levels, ASSETS_FILE) ? 2 : -2;
            if (editor->saved == 2) {
                *editor->current_level = 0;
                editor->layer = 0;
                editor->selected = -1;
            }
            return;
        }
    }
    if (mouse.x >= viewport.x * (1 - EDITOR_PANEL_WIDTH / GetScreenWidth())) {
        return;
    }
    float wheel = GetMouseWheelMove();
    if (wheel) {
        editor->camera.zoom = fmaxf(EDITOR_ZOOM_MIN, fminf(EDITOR_ZOOM_MAX,
                            editor->camera.zoom + wheel * EDITOR_ZOOM_STEP));
    }
    Vector2 world = GetScreenToWorld2D(mouse, editor->camera);
    float x = floorf(world.x / level->tile_size) * level->tile_size;
    float y = floorf(world.y / level->tile_size) * level->tile_size;
    if (x < 0 || y < 0 || x >= WORLD_SIZE || y >= WORLD_SIZE ||
        x >= level->width * level->tile_size || y >= level->height * level->tile_size) {
            return;
        }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        int hit = -1;
        for (int i = arrlen(level->entities) - 1; i >= 0; --i) {
            if (level->entities[i].layer == editor->layer &&
                CheckCollisionPointRec(world, entity_bounds(&level->entities[i], level->tile_size))) {
                hit = i;
                break;
            }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (hit >= 0) {
                entity_free(&level->entities[hit]);
                arrdel(level->entities, hit);
                editor->saved = 0;
            }
            editor->selected = -1;
            return;
        }
        if (hit >= 0) {
            editor->selected = hit;
            editor->layer = level->entities[hit].layer;
        } else if (editor->brush >= 0 && editor->brush < arrlen(editor->templates)) {
            struct entity e = entity_copy(&editor->templates[editor->brush]);
            if (e.is_active) {
                e.position = (Vector2){x, y};
                e.layer = editor->layer;
                editor->selected = level_add_entity(level, e);
                if (editor->selected < 0) {
                    entity_free(&e);
                }
                editor->saved = 0;
            }
        }
    }
}

void level_editor_draw(const struct level_editor *editor, Camera2D camera)
{
    if (!editor->levels || !arrlen(*editor->levels)) {
        return;
    }
    const struct level *level = &(*editor->levels)[*editor->current_level];
    BeginMode2D(camera);
        for (int layer = 0; layer < arrlen(level->layers); ++layer) {
            if (!level->layers[layer].visible) {
                continue;
            }
            for (int i = 0; i < arrlen(level->entities); ++i) {
                const struct entity *e = &level->entities[i];
                if (!e->is_active || e->layer != (uint32_t)layer) {
                    continue;
                }
                float opacity = editor->enabled && (uint32_t)layer != editor->layer ? EDITOR_INACTIVE_OPACITY : 1.0f;
                entity_draw(e, opacity);
                if (editor->enabled && editor->show_colliders && arrlen(e->collider.vertices) >= 3) {
                    Color color = Fade(e->is_solid ? ORANGE : PURPLE, opacity);
                    for (int v = 0; v < arrlen(e->collider.vertices); ++v) {
                        Vector2 a = e->collider.vertices[v];
                        Vector2 b = e->collider.vertices[(v + 1) % arrlen(e->collider.vertices)];
                        Vector2 p = {e->position.x + a.x, e->position.y + a.y};
                        DrawLineV(p, (Vector2){e->position.x + b.x, e->position.y + b.y}, color);
                        DrawCircleV(p, 1.5f, color);
                    }
                }
                if (editor->enabled && editor->selected == i) {
                    Rectangle r = entity_bounds(e, level->tile_size);
                    DrawRectangleLinesEx(r, 1.0f, YELLOW);
                }
            }
        }
        if (editor->enabled && editor->grid) {
            for (uint32_t x = 0; x <= level->width * level->tile_size; x += level->tile_size) {
                DrawLine(x, 0, x, level->height * level->tile_size, Fade(LIGHTGRAY, 0.3f));
            }
            for (uint32_t y = 0; y <= level->height * level->tile_size; y += level->tile_size) {
                DrawLine(0, y, level->width * level->tile_size, y, Fade(LIGHTGRAY, 0.3f));
            }
        }
    EndMode2D();
}

void level_editor_gui(struct level_editor *editor)
{
    if (!editor->enabled || !editor->levels || !arrlen(*editor->levels)) {
        return;
    }
    struct level *level = &(*editor->levels)[*editor->current_level];
    if (!arrlen(level->layers)) {
        return;
    }
    float x = GetScreenWidth() - EDITOR_PANEL_WIDTH;
    float height = GetScreenHeight();
    Rectangle panel = {x, 0, EDITOR_PANEL_WIDTH, height};
    GuiPanel(panel, NULL);
    GuiLabel((Rectangle){x + 10, 8, 150, 20}, "LEVEL EDITOR");
    if (GuiButton((Rectangle){x + 174, 8, 66, 22}, "Save")) {
        editor->saved = level_save(*editor->levels, ASSETS_FILE) ? 1 : -1;
    }

    int place_layer = editor->layer;
    GuiLabel((Rectangle){x + 10, 40, 120, 20}, "Place on layer");
    GuiSpinner((Rectangle){x + 140, 40, 98, 22}, NULL, &place_layer, 0, arrlen(level->layers) - 1, false);
    editor->layer = place_layer;
    bool layer_visible = level->layers[editor->layer].visible;
    GuiCheckBox((Rectangle){x + 10, 70, 16, 16}, "Layer visible", &layer_visible);
    GuiCheckBox((Rectangle){x + 123, 70, 16, 16}, "Show Colliders", &editor->show_colliders);
    if (layer_visible != (bool)level->layers[editor->layer].visible) {
        level->layers[editor->layer].visible = layer_visible;
        editor->saved = 0;
    }

    GuiLabel((Rectangle){x + 10, 98, 220, 18}, TextFormat("TEMPLATES  (%d)", arrlen(editor->templates)));
    const float list_y = 120;
    const float list_height = fmaxf(EDITOR_TEMPLATE_CELL, height - EDITOR_INSPECTOR_HEIGHT - list_y);
    const int columns = (int)((EDITOR_PANEL_WIDTH - 20) / EDITOR_TEMPLATE_CELL);
    const int rows = (int)(list_height / EDITOR_TEMPLATE_CELL);
    const int slots = columns * rows;
    const int count = arrlen(editor->templates);
    const int max_start = (count > slots) ? ((count - slots + columns - 1) / columns) * columns : 0;
    if ((int)editor->palette_start > max_start) {
        editor->palette_start = max_start;
    }
    if (GuiButton((Rectangle){x + 184, 98, 25, 20}, "<") && editor->palette_start >= (uint32_t)columns) {
        editor->palette_start -= columns;
    }
    if (GuiButton((Rectangle){x + 214, 98, 25, 20}, ">") && editor->palette_start < (uint32_t)max_start) {
        editor->palette_start += columns;
    }
    Rectangle list = {x + 10, list_y, columns * EDITOR_TEMPLATE_CELL, rows * EDITOR_TEMPLATE_CELL};
    if (CheckCollisionPointRec(GetMousePosition(), list)) {
        float wheel = GetMouseWheelMove();
        int offset = (int)editor->palette_start - (int)wheel * columns;
        if (wheel && offset < 0) {
            offset = 0;
        }
        if (wheel && offset > max_start) {
            offset = max_start;
        }
        if (wheel) {
            editor->palette_start = offset;
        }
    }
    for (int i = 0; i < slots; ++i) {
        int index = editor->palette_start + i;
        if (index >= count) {
            break;
        }
        Rectangle cell = {list.x + (i % columns) * EDITOR_TEMPLATE_CELL,
                          list.y + (i / columns) * EDITOR_TEMPLATE_CELL,
                          EDITOR_TEMPLATE_CELL - 2, EDITOR_TEMPLATE_CELL - 2};
        if (GuiButton(cell, "")) {
            editor->brush = index;
            editor->selected = -1;
        }
        editor_preview(editor->templates[index].sp, cell);
        if (editor->brush == index) {
            DrawRectangleLinesEx(cell, 2, YELLOW);
        }
    }
    float y = height - EDITOR_INSPECTOR_HEIGHT + 8;
    GuiLine((Rectangle){x + 10, y, 230, 18}, NULL);
    y += 20;
    GuiLabel((Rectangle){x + 10, y, 220, 18}, TextFormat("Brush: %d", editor->brush));
    y += 22;
    if (editor->selected < 0 || editor->selected >= arrlen(level->entities)) {
        GuiLabel((Rectangle){x + 10, y, 225, 20}, "Select an entity to edit");
        return;
    }
    struct entity *e = &level->entities[editor->selected];
    GuiLabel((Rectangle){x + 10, y, 225, 20}, TextFormat("ENTITY #%llu", (unsigned long long)e->id));
    y += 28;
    int entity_layer = e->layer;
    GuiLabel((Rectangle){x + 10, y, 100, 20}, "Layer");
    GuiSpinner((Rectangle){x + 140, y, 98, 22}, NULL, &entity_layer, 0, arrlen(level->layers) - 1, false);
    if (entity_layer != (int)e->layer) {
        e->layer = entity_layer;
        editor->layer = entity_layer;
        editor->saved = 0;
    }
    y += 29;
    bool visible = e->is_visible;
    bool solid = e->is_solid;
    GuiCheckBox((Rectangle){x + 10, y, 16, 16}, "Visible", &visible);
    GuiCheckBox((Rectangle){x + 130, y, 16, 16}, "Solid", &solid);
    if (visible != e->is_visible || solid != e->is_solid) {
        e->is_visible = visible;
        if (solid && !arrlen(e->collider.vertices)) {
            Rectangle bounds = entity_bounds(e, level->tile_size);
            e->collider = physics_polygon_rectangle((Rectangle){0, 0, bounds.width, bounds.height});
            solid = arrlen(e->collider.vertices) != 0;
        }
        e->is_solid = solid;
        editor->saved = 0;
    }
    y += 28;
    int hp = e->health;
    GuiLabel((Rectangle){x + 10, y, 100, 20}, "Health");
    GuiSpinner((Rectangle){x + 140, y, 98, 22}, NULL, &hp, 0, INT32_MAX, false);
    if (hp != e->health) {
        e->health = hp;
        editor->saved = 0;
    }
    y += 28;
    int damage = (e->damage > INT32_MAX) ? INT32_MAX : (int)e->damage;
    GuiLabel((Rectangle){x + 10, y, 100, 20}, "Damage");
    int previous_damage = damage;
    GuiSpinner((Rectangle){x + 140, y, 98, 22}, NULL, &damage, 0, INT32_MAX, false);
    if (damage != previous_damage) {
        e->damage = damage;
        editor->saved = 0;
    }
}
