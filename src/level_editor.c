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
#define EDITOR_COLLAPSED_WIDTH 30.0f
#define EDITOR_TEMPLATE_CELL 36.0f
#define EDITOR_INACTIVE_OPACITY 0.4f
#define EDITOR_VERTEX_RADIUS 5.0f

enum { EDITOR_SELECT, EDITOR_PLACE, EDITOR_POLYGON };
enum { EDITOR_OBJECTS, EDITOR_LAYERS, EDITOR_PROPERTIES };
enum { EDITOR_DRAG_NONE, EDITOR_DRAG_MOVE, EDITOR_DRAG_BOX, EDITOR_DRAG_VERTEX };

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

static bool editor_inside_level(const struct entity *e, Vector2 position, const struct level *level)
{
    Rectangle bounds = entity_bounds(e, level->tile_size);
    return position.x >= 0 && position.y >= 0 &&
           position.x + bounds.width <= fminf(WORLD_SIZE, level->width * level->tile_size) &&
           position.y + bounds.height <= fminf(WORLD_SIZE, level->height * level->tile_size);
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

static void editor_clear_selection(struct level_editor *editor)
{
    if (editor->selection) {
        arrsetlen(editor->selection, 0);
    }
}

static bool editor_is_selected(const struct level_editor *editor, uint64_t id)
{
    for (int i = 0; i < arrlen(editor->selection); ++i) {
        if (editor->selection[i] == id) {
            return true;
        }
    }
    return false;
}

static int editor_selected_index(const struct level_editor *editor, const struct level *level)
{
    if (arrlen(editor->selection) != 1) {
        return -1;
    }
    for (int i = 0; i < arrlen(level->entities); ++i) {
        if (level->entities[i].id == editor->selection[0]) {
            return i;
        }
    }
    return -1;
}

static void editor_select(struct level_editor *editor, uint64_t id, bool toggle)
{
    if (!toggle) {
        editor_clear_selection(editor);
    }
    if (toggle && editor_is_selected(editor, id)) {
        for (int i = 0; i < arrlen(editor->selection); ++i) {
            if (editor->selection[i] == id) {
                arrdel(editor->selection, i);
                break;
            }
        }
    } else {
        arrput(editor->selection, id);
    }
}

static void editor_clear_clipboard(struct level_editor *editor)
{
    for (int i = 0; i < arrlen(editor->clipboard); ++i) {
        entity_free(&editor->clipboard[i]);
    }
    arrfree(editor->clipboard);
}

static void editor_copy_selection(struct level_editor *editor, const struct level *level)
{
    if (!arrlen(editor->selection)) {
        return;
    }
    editor_clear_clipboard(editor);
    for (int i = 0; i < arrlen(level->entities); ++i) {
        if (editor_is_selected(editor, level->entities[i].id)) {
            struct entity e = entity_copy(&level->entities[i]);
            if (e.is_active) {
                arrput(editor->clipboard, e);
            }
        }
    }
}

static void editor_paste(struct level_editor *editor, struct level *level, Vector2 position)
{
    if (!arrlen(editor->clipboard)) {
        return;
    }
    Vector2 anchor = editor->clipboard[0].position;
    for (int i = 1; i < arrlen(editor->clipboard); ++i) {
        anchor.x = fminf(anchor.x, editor->clipboard[i].position.x);
        anchor.y = fminf(anchor.y, editor->clipboard[i].position.y);
    }
    editor_clear_selection(editor);
    for (int i = 0; i < arrlen(editor->clipboard); ++i) {
        const struct entity *src = &editor->clipboard[i];
        Vector2 at = {position.x + src->position.x - anchor.x,
                      position.y + src->position.y - anchor.y};
        if (!editor_inside_level(src, at, level)) {
            continue;
        }
        bool exists = false;
        if (src->unique) {
            for (int j = 0; j < arrlen(level->entities); ++j) {
                if (level->entities[j].type == src->type) {
                    exists = true;
                    break;
                }
            }
        }
        if (exists) {
            continue;
        }
        struct entity e = entity_copy(src);
        if (!e.is_active) {
            continue;
        }
        e.position = at;
        e.layer = editor->layer;
        e.id = 0;
        e.owner_id = 0;
        int index = level_add_entity(level, e);
        if (index < 0) {
            entity_free(&e);
        } else {
            arrput(editor->selection, level->entities[index].id);
            editor->saved = 0;
        }
    }
}

static bool editor_polygon_valid(const Vector2 *vertices)
{
    int count = arrlen(vertices);
    if (count < 3) {
        return false;
    }
    float winding = 0;
    for (int i = 0; i < count; ++i) {
        Vector2 a = vertices[i];
        Vector2 b = vertices[(i + 1) % count];
        Vector2 c = vertices[(i + 2) % count];
        float dx = b.x - a.x, dy = b.y - a.y;
        float cross = dx * (c.y - b.y) - dy * (c.x - b.x);
        if (!isfinite(a.x) || !isfinite(a.y) || dx * dx + dy * dy < 0.0001f ||
            (cross > 0.0001f && winding < 0) || (cross < -0.0001f && winding > 0)) {
            return false;
        }
        winding += cross;
    }
    return fabsf(winding) > 0.0001f;
}

static bool editor_polygon_insert(struct physics_polygon *polygon, Vector2 point, float radius, int *vertex)
{
    if (arrlenu(polygon->vertices) >= LEVEL_MAX_POLYGON_VERTICES) {
        return false;
    }
    for (int i = 0; i < arrlen(polygon->vertices); ++i) {
        Vector2 a = polygon->vertices[i];
        Vector2 b = polygon->vertices[(i + 1) % arrlen(polygon->vertices)];
        float dx = b.x - a.x, dy = b.y - a.y;
        float len = dx * dx + dy * dy;
        if (len <= 0) {
            continue;
        }
        float t = ((point.x - a.x) * dx + (point.y - a.y) * dy) / len;
        if (t <= 0.01f || t >= 0.99f) {
            continue;
        }
        Vector2 projected = {a.x + dx * t, a.y + dy * t};
        if ((projected.x - point.x) * (projected.x - point.x) +
            (projected.y - point.y) * (projected.y - point.y) <= radius * radius) {
            arrins(polygon->vertices, i + 1, projected);
            if (!editor_polygon_valid(polygon->vertices)) {
                arrdel(polygon->vertices, i + 1);
                return false;
            }
            *vertex = i + 1;
            return true;
        }
    }
    return false;
}

void level_editor_update(struct level_editor *editor, Vector2 mouse, Vector2 viewport)
{
    if (IsKeyPressed(KEY_F1)) {
        editor->enabled = !editor->enabled;
        editor_clear_selection(editor);
        editor->tool = EDITOR_SELECT;
        editor->drag_mode = EDITOR_DRAG_NONE;
    }
    if (!editor->enabled || !editor->levels || !arrlen(*editor->levels)) {
        return;
    }
    if (IsKeyPressed(KEY_F2)) {
        editor->sidebar_collapsed = !editor->sidebar_collapsed;
    }
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    float pan = EDITOR_PAN_SPEED * GetFrameTime() / editor->camera.zoom;
    if (!ctrl) {
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
        editor_clear_selection(editor);
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
        editor_clear_selection(editor);
        editor->tool = EDITOR_SELECT;
    }
    if (IsKeyPressed(KEY_G)) {
        editor->grid = !editor->grid;
    }
    if (IsKeyPressed(KEY_V) && !ctrl) {
        level->layers[editor->layer].visible = !level->layers[editor->layer].visible;
        editor->saved = 0;
    }
    if (IsKeyPressed(KEY_PAGE_DOWN) && *editor->current_level + 1 < arrlenu(*editor->levels)) {
        ++*editor->current_level;
        editor_clear_selection(editor);
        editor->tool = EDITOR_SELECT;
        editor->drag_mode = EDITOR_DRAG_NONE;
        return;
    }
    if (IsKeyPressed(KEY_PAGE_UP) && *editor->current_level > 0) {
        --*editor->current_level;
        editor_clear_selection(editor);
        editor->tool = EDITOR_SELECT;
        editor->drag_mode = EDITOR_DRAG_NONE;
        return;
    }
    if (ctrl && IsKeyPressed(KEY_S)) {
        editor->saved = level_save(*editor->levels, ASSETS_FILE) ? 1 : -1;
    }
    if (ctrl && IsKeyPressed(KEY_O)) {
        editor->saved = level_load(editor->levels, ASSETS_FILE) ? 2 : -2;
        if (editor->saved == 2) {
            *editor->current_level = 0;
            editor->layer = 0;
            editor_clear_selection(editor);
            editor->tool = EDITOR_SELECT;
            editor->drag_mode = EDITOR_DRAG_NONE;
        }
        return;
    }
    if (ctrl && IsKeyPressed(KEY_C)) {
        editor_copy_selection(editor, level);
    }
    if (ctrl && IsKeyPressed(KEY_D)) {
        struct entity *previous = editor->clipboard;
        editor->clipboard = NULL;
        editor_copy_selection(editor, level);
        if (arrlen(editor->clipboard)) {
            Vector2 anchor = editor->clipboard[0].position;
            for (int i = 1; i < arrlen(editor->clipboard); ++i) {
                anchor.x = fminf(anchor.x, editor->clipboard[i].position.x);
                anchor.y = fminf(anchor.y, editor->clipboard[i].position.y);
            }
            editor_paste(editor, level, (Vector2){anchor.x + level->tile_size, anchor.y + level->tile_size});
        }
        editor_clear_clipboard(editor);
        editor->clipboard = previous;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (editor->tool == EDITOR_POLYGON) {
            editor->tool = EDITOR_SELECT;
            editor->vertex = -1;
        } else {
            editor_clear_selection(editor);
        }
        editor->drag_mode = EDITOR_DRAG_NONE;
    }
    if (IsKeyPressed(KEY_DELETE)) {
        int index = editor_selected_index(editor, level);
        if (editor->tool == EDITOR_POLYGON && index >= 0 && editor->vertex >= 0) {
            Vector2 *vertices = level->entities[index].collider.vertices;
            if (arrlen(vertices) > 3) {
                int vertex = editor->vertex;
                Vector2 original = vertices[vertex];
                arrdel(vertices, vertex);
                if (!editor_polygon_valid(vertices)) {
                    arrins(vertices, vertex, original);
                } else {
                    editor->saved = 0;
                    editor->vertex = -1;
                }
                level->entities[index].collider.vertices = vertices;
            }
        } else if (editor->tool != EDITOR_POLYGON) {
            for (int i = arrlen(level->entities) - 1; i >= 0; --i) {
                if (editor_is_selected(editor, level->entities[i].id)) {
                    entity_free(&level->entities[i]);
                    arrdel(level->entities, i);
                    editor->saved = 0;
                }
            }
            editor_clear_selection(editor);
        }
    }

    float panel_width = editor->sidebar_collapsed ? EDITOR_COLLAPSED_WIDTH : EDITOR_PANEL_WIDTH;
    bool over_panel = mouse.x >= viewport.x * (1 - panel_width / GetScreenWidth());
    Vector2 world = GetScreenToWorld2D(mouse, editor->camera);
    if (editor->drag_mode && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (editor->drag_mode == EDITOR_DRAG_BOX) {
            Rectangle box = {fminf(editor->drag_start.x, world.x), fminf(editor->drag_start.y, world.y),
                             fabsf(world.x - editor->drag_start.x), fabsf(world.y - editor->drag_start.y)};
            for (int i = 0; i < arrlen(level->entities); ++i) {
                const struct entity *e = &level->entities[i];
                if (e->is_active && e->layer == editor->layer && level->layers[e->layer].visible &&
                    CheckCollisionRecs(box, entity_bounds(e, level->tile_size)) &&
                    !editor_is_selected(editor, e->id)) {
                    arrput(editor->selection, e->id);
                }
            }
        }
        editor->drag_mode = EDITOR_DRAG_NONE;
    }
    if (over_panel) {
        return;
    }
    float wheel = GetMouseWheelMove();
    if (wheel) {
        editor->camera.zoom = fmaxf(EDITOR_ZOOM_MIN, fminf(EDITOR_ZOOM_MAX,
                            editor->camera.zoom + wheel * EDITOR_ZOOM_STEP));
    }
    float x = floorf(world.x / level->tile_size) * level->tile_size;
    float y = floorf(world.y / level->tile_size) * level->tile_size;
    if (ctrl && IsKeyPressed(KEY_V)) {
        editor_paste(editor, level, (Vector2){x, y});
        editor->tool = EDITOR_SELECT;
    }
    if (editor->drag_mode == EDITOR_DRAG_MOVE && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 snapped = {floorf(world.x / level->tile_size) * level->tile_size,
                           floorf(world.y / level->tile_size) * level->tile_size};
        Vector2 delta = {snapped.x - editor->drag_current.x, snapped.y - editor->drag_current.y};
        if (delta.x || delta.y) {
            for (int i = 0; i < arrlen(level->entities); ++i) {
                struct entity *e = &level->entities[i];
                if (editor_is_selected(editor, e->id)) {
                    Vector2 next = {e->position.x + delta.x, e->position.y + delta.y};
                    if (!editor_inside_level(e, next, level)) {
                        return;
                    }
                }
            }
            for (int i = 0; i < arrlen(level->entities); ++i) {
                if (editor_is_selected(editor, level->entities[i].id)) {
                    level->entities[i].position.x += delta.x;
                    level->entities[i].position.y += delta.y;
                }
            }
            editor->drag_current = snapped;
            editor->saved = 0;
        }
        return;
    }
    if (editor->drag_mode == EDITOR_DRAG_VERTEX && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        int index = editor_selected_index(editor, level);
        if (index >= 0 && editor->vertex >= 0) {
            Vector2 *vertices = level->entities[index].collider.vertices;
            Vector2 original = vertices[editor->vertex];
            vertices[editor->vertex] = (Vector2){world.x - level->entities[index].position.x,
                                                  world.y - level->entities[index].position.y};
            if (!editor_polygon_valid(vertices)) {
                vertices[editor->vertex] = original;
            } else {
                editor->saved = 0;
            }
        }
        return;
    }
    if (editor->drag_mode == EDITOR_DRAG_BOX && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        editor->drag_current = world;
        return;
    }
    if (x < 0 || y < 0 || x >= WORLD_SIZE || y >= WORLD_SIZE ||
        x >= level->width * level->tile_size || y >= level->height * level->tile_size) {
        return;
    }
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        return;
    }
    if (editor->tool == EDITOR_POLYGON) {
        int index = editor_selected_index(editor, level);
        if (index < 0 || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            return;
        }
        struct entity *e = &level->entities[index];
        float radius = EDITOR_VERTEX_RADIUS / editor->camera.zoom;
        Vector2 local = {world.x - e->position.x, world.y - e->position.y};
        editor->vertex = -1;
        for (int i = 0; i < arrlen(e->collider.vertices); ++i) {
            Vector2 p = e->collider.vertices[i];
            if (fabsf(local.x - p.x) <= radius && fabsf(local.y - p.y) <= radius) {
                editor->vertex = i;
                break;
            }
        }
        if (editor->vertex < 0 && arrlen(e->collider.vertices) >= 3 &&
            editor_polygon_insert(&e->collider, local, radius, &editor->vertex)) {
            editor->saved = 0;
        }
        if (editor->vertex >= 0) {
            editor->drag_mode = EDITOR_DRAG_VERTEX;
        }
        return;
    }
    int hit = -1;
    for (int i = arrlen(level->entities) - 1; i >= 0; --i) {
        if (level->entities[i].is_active && level->entities[i].layer == editor->layer &&
            level->layers[editor->layer].visible &&
            CheckCollisionPointRec(world, entity_bounds(&level->entities[i], level->tile_size))) {
            hit = i;
            break;
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        if (hit >= 0) {
            uint64_t id = level->entities[hit].id;
            entity_free(&level->entities[hit]);
            arrdel(level->entities, hit);
            for (int i = 0; i < arrlen(editor->selection); ++i) {
                if (editor->selection[i] == id) {
                    arrdel(editor->selection, i);
                    break;
                }
            }
            editor->saved = 0;
        }
        return;
    }
    if (hit >= 0) {
        uint64_t id = level->entities[hit].id;
        editor->sidebar_tab = EDITOR_PROPERTIES;
        if (shift) {
            editor_select(editor, id, true);
        } else if (!editor_is_selected(editor, id)) {
            editor_select(editor, id, false);
        }
        if (editor_is_selected(editor, id)) {
            editor->drag_mode = EDITOR_DRAG_MOVE;
            editor->drag_current = (Vector2){x, y};
        }
    } else if (editor->tool == EDITOR_PLACE && editor->brush >= 0 &&
               editor->brush < arrlen(editor->templates)) {
        struct entity e = entity_copy(&editor->templates[editor->brush]);
        if (e.is_active) {
            e.position = (Vector2){x, y};
            e.layer = editor->layer;
            if (!editor_inside_level(&e, e.position, level)) {
                entity_free(&e);
            } else {
                int index = level_add_entity(level, e);
                if (index < 0) {
                    entity_free(&e);
                } else {
                    editor_select(editor, level->entities[index].id, false);
                    editor->saved = 0;
                }
            }
        }
    } else {
        if (!shift) {
            editor_clear_selection(editor);
        }
        editor->drag_mode = EDITOR_DRAG_BOX;
        editor->drag_start = world;
        editor->drag_current = world;
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
                if (editor->enabled && (editor->show_colliders || (editor->tool == EDITOR_POLYGON && editor_is_selected(editor, e->id))) && arrlen(e->collider.vertices) >= 3) {
                    Color color = Fade(e->is_solid ? ORANGE : PURPLE, opacity);
                    for (int v = 0; v < arrlen(e->collider.vertices); ++v) {
                        Vector2 a = e->collider.vertices[v];
                        Vector2 b = e->collider.vertices[(v + 1) % arrlen(e->collider.vertices)];
                        Vector2 p = {e->position.x + a.x, e->position.y + a.y};
                        DrawLineV(p, (Vector2){e->position.x + b.x, e->position.y + b.y}, color);
                        DrawCircleV(p, 1.5f, color);
                    }
                }
                if (editor->enabled && editor_is_selected(editor, e->id)) {
                    Rectangle r = entity_bounds(e, level->tile_size);
                    DrawRectangleLinesEx(r, 1.0f, YELLOW);
                    if (editor->tool == EDITOR_POLYGON) {
                        for (int v = 0; v < arrlen(e->collider.vertices); ++v) {
                            Vector2 p = e->collider.vertices[v];
                            DrawCircleV((Vector2){e->position.x + p.x, e->position.y + p.y},
                                        EDITOR_VERTEX_RADIUS / camera.zoom,
                                        v == editor->vertex ? YELLOW : ORANGE);
                        }
                    }
                }
            }
        }
        if (editor->enabled && editor->drag_mode == EDITOR_DRAG_BOX) {
            Rectangle box = {fminf(editor->drag_start.x, editor->drag_current.x),
                             fminf(editor->drag_start.y, editor->drag_current.y),
                             fabsf(editor->drag_current.x - editor->drag_start.x),
                             fabsf(editor->drag_current.y - editor->drag_start.y)};
            DrawRectangleLinesEx(box, 1.0f / camera.zoom, YELLOW);
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
    float height = GetScreenHeight();
    float width = editor->sidebar_collapsed ? EDITOR_COLLAPSED_WIDTH : EDITOR_PANEL_WIDTH;
    float x = GetScreenWidth() - width;
    GuiPanel((Rectangle){x, 0, width, height}, NULL);
    if (editor->sidebar_collapsed) {
        if (GuiButton((Rectangle){x + 3, 8, 24, 24}, "<")) {
            editor->sidebar_collapsed = false;
        }
        return;
    }
    GuiLabel((Rectangle){x + 10, 8, 140, 22}, "LEVEL EDITOR");
    if (GuiButton((Rectangle){x + 154, 8, 24, 22}, ">")) {
        editor->sidebar_collapsed = true;
        return;
    }
    if (GuiButton((Rectangle){x + 184, 8, 56, 22}, "Save")) {
        editor->saved = level_save(*editor->levels, ASSETS_FILE) ? 1 : -1;
    }

    int tool = editor->tool;
    GuiToggleGroup((Rectangle){x + 10, 40, (230 - 2 * GuiGetStyle(TOGGLE, GROUP_PADDING)) / 3.0f, 22},
                   "Select;Place;Polygon", &tool);
    if (tool != editor->tool &&
        (tool != EDITOR_POLYGON || editor_selected_index(editor, level) >= 0)) {
        editor->tool = tool;
        editor->vertex = -1;
        editor->drag_mode = EDITOR_DRAG_NONE;
    }

    GuiToggleGroup((Rectangle){x + 10, 73, (230 - 2 * GuiGetStyle(TOGGLE, GROUP_PADDING)) / 3.0f, 22},
                   "Objects;Layers;Props", &editor->sidebar_tab);
    if (editor->sidebar_tab == EDITOR_OBJECTS) {
        GuiLabel((Rectangle){x + 10, 107, 166, 18}, TextFormat("TEMPLATES (%d)", arrlen(editor->templates)));
        const int columns = (int)((EDITOR_PANEL_WIDTH - 20) / EDITOR_TEMPLATE_CELL);
        const int rows = (int)(fmaxf(0, height - 136) / EDITOR_TEMPLATE_CELL);
        const int slots = columns * rows;
        const int count = arrlen(editor->templates);
        const int max_start = (count > slots) ? ((count - slots + columns - 1) / columns) * columns : 0;
        if ((int)editor->palette_start > max_start) {
            editor->palette_start = max_start;
        }
        if (GuiButton((Rectangle){x + 184, 105, 25, 20}, "<") && editor->palette_start >= (uint32_t)columns) {
            editor->palette_start -= columns;
        }
        if (GuiButton((Rectangle){x + 214, 105, 25, 20}, ">") && editor->palette_start < (uint32_t)max_start) {
            editor->palette_start += columns;
        }
        Rectangle list = {x + 10, 132, columns * EDITOR_TEMPLATE_CELL, rows * EDITOR_TEMPLATE_CELL};
        if (CheckCollisionPointRec(GetMousePosition(), list)) {
            int offset = (int)editor->palette_start - (int)GetMouseWheelMove() * columns;
            if (offset < 0) {
                offset = 0;
            }
            if (offset > max_start) {
                offset = max_start;
            }
            editor->palette_start = offset;
        }
        for (int i = 0; i < slots && (int)editor->palette_start + i < count; ++i) {
            int index = editor->palette_start + i;
            Rectangle cell = {list.x + (i % columns) * EDITOR_TEMPLATE_CELL,
                              list.y + (i / columns) * EDITOR_TEMPLATE_CELL,
                              EDITOR_TEMPLATE_CELL - 2, EDITOR_TEMPLATE_CELL - 2};
            if (GuiButton(cell, "")) {
                editor->brush = index;
                editor->tool = EDITOR_PLACE;
                editor_clear_selection(editor);
            }
            editor_preview(editor->templates[index].sp, cell);
            if (editor->brush == index) {
                DrawRectangleLinesEx(cell, 2, YELLOW);
            }
        }
    } else if (editor->sidebar_tab == EDITOR_LAYERS) {
        int place_layer = editor->layer;
        GuiLabel((Rectangle){x + 10, 112, 120, 20}, "Active layer");
        GuiSpinner((Rectangle){x + 140, 112, 98, 22}, NULL, &place_layer, 0, arrlen(level->layers) - 1, false);
        if (editor->layer != (uint32_t)place_layer) {
            editor->layer = place_layer;
            editor_clear_selection(editor);
            editor->tool = EDITOR_SELECT;
        }
        bool visible = level->layers[editor->layer].visible;
        GuiCheckBox((Rectangle){x + 10, 149, 16, 16}, "Layer visible", &visible);
        GuiCheckBox((Rectangle){x + 10, 180, 16, 16}, "Show Colliders", &editor->show_colliders);
        if (visible != (bool)level->layers[editor->layer].visible) {
            level->layers[editor->layer].visible = visible;
            editor->saved = 0;
        }
        GuiLabel((Rectangle){x + 10, 214, 225, 20}, "Tab: next layer  |  G: grid");
    } else {
        int index = editor_selected_index(editor, level);
        GuiLabel((Rectangle){x + 10, 108, 225, 20}, TextFormat("Selected: %d", arrlen(editor->selection)));
        if (index < 0) {
            GuiLabel((Rectangle){x + 10, 140, 225, 20},
                     arrlen(editor->selection) > 1 ? "Group selected" : "Select an entity to edit");
            return;
        }
        struct entity *e = &level->entities[index];
        GuiLabel((Rectangle){x + 10, 141, 225, 20}, TextFormat("ENTITY #%llu", (unsigned long long)e->id));
        int entity_layer = e->layer;
        GuiLabel((Rectangle){x + 10, 177, 100, 20}, "Layer");
        GuiSpinner((Rectangle){x + 140, 177, 98, 22}, NULL, &entity_layer, 0, arrlen(level->layers) - 1, false);
        if (entity_layer != (int)e->layer) {
            e->layer = entity_layer;
            editor->layer = entity_layer;
            editor->saved = 0;
        }
        bool visible = e->is_visible;
        bool solid = e->is_solid;
        GuiCheckBox((Rectangle){x + 10, 215, 16, 16}, "Visible", &visible);
        GuiCheckBox((Rectangle){x + 130, 215, 16, 16}, "Solid", &solid);
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
        int hp = e->health;
        GuiLabel((Rectangle){x + 10, 250, 100, 20}, "Health");
        GuiSpinner((Rectangle){x + 140, 250, 98, 22}, NULL, &hp, 0, INT32_MAX, false);
        if (hp != e->health) {
            e->health = hp;
            editor->saved = 0;
        }
        int damage = (e->damage > INT32_MAX) ? INT32_MAX : (int)e->damage;
        int previous_damage = damage;
        GuiLabel((Rectangle){x + 10, 284, 100, 20}, "Damage");
        GuiSpinner((Rectangle){x + 140, 284, 98, 22}, NULL, &damage, 0, INT32_MAX, false);
        if (damage != previous_damage) {
            e->damage = damage;
            editor->saved = 0;
        }
    }
}
