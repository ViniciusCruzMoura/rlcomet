#include "level.h"
#include "game.h"
#include "assets.h"
#include "stb_ds.h"
#include "mpack/mpack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEVEL_VERSION 7u
#define LEVEL_MAX_ROWS 32u
#define LEVEL_MAX_FRAMES 4096u

static bool put32(FILE *f, uint32_t n)
{
    unsigned char b[4] = {n, n >> 8, n >> 16, n >> 24};
    return fwrite(b, 1, 4, f) == 4;
}

static bool get32(FILE *f, uint32_t *n)
{
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) {
        return false;
    }
    *n = (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
    return true;
}

static bool chunk(FILE *f, const char tag[4], uint32_t size)
{
    return fwrite(tag, 1, 4, f) == 4 && put32(f, size);
}

static void write_vec2(mpack_writer_t *w, Vector2 v)
{
    mpack_start_array(w, 2);
    mpack_write_float(w, v.x);
    mpack_write_float(w, v.y);
    mpack_finish_array(w);
}

static bool read_vec2(mpack_node_t node, Vector2 *v)
{
    if (mpack_node_array_length(node) != 2) {
        return false;
    }
    v->x = mpack_node_float(mpack_node_array_at(node, 0));
    v->y = mpack_node_float(mpack_node_array_at(node, 1));
    return true;
}

static void write_rect(mpack_writer_t *w, Rectangle r)
{
    mpack_start_array(w, 4);
    mpack_write_float(w, r.x);
    mpack_write_float(w, r.y);
    mpack_write_float(w, r.width);
    mpack_write_float(w, r.height);
    mpack_finish_array(w);
}

static bool read_rect(mpack_node_t node, Rectangle *r)
{
    if (mpack_node_array_length(node) != 4) {
        return false;
    }
    r->x = mpack_node_float(mpack_node_array_at(node, 0));
    r->y = mpack_node_float(mpack_node_array_at(node, 1));
    r->width = mpack_node_float(mpack_node_array_at(node, 2));
    r->height = mpack_node_float(mpack_node_array_at(node, 3));
    return true;
}

static bool write_entity(mpack_writer_t *w, const struct entity *e)
{
    if (e->type > ENEMY || e->sp.texture_id >= ASSETS_MAX_TEXTURES ||
        e->sp.total_rows > LEVEL_MAX_ROWS ||
        (e->sp.total_rows && (!e->sp.frames_per_row || e->sp.current_row >= e->sp.total_rows)) ||
        arrlenu(e->collider.vertices) > LEVEL_MAX_POLYGON_VERTICES ||
        (arrlen(e->collider.vertices) && arrlen(e->collider.vertices) < 3)) {
        return false;
    }
    mpack_build_map(w);
    mpack_write_cstr(w, "type");       mpack_write_u32(w, e->type);
    mpack_write_cstr(w, "layer");      mpack_write_u32(w, e->layer);
    mpack_write_cstr(w, "position");   write_vec2(w, e->position);
    mpack_write_cstr(w, "speed");      write_vec2(w, e->speed);
    mpack_write_cstr(w, "facing");     write_vec2(w, e->facing);
    mpack_write_cstr(w, "lifetime");   mpack_write_u32(w, e->lifetime);
    mpack_write_cstr(w, "visible");    mpack_write_bool(w, e->is_visible);
    mpack_write_cstr(w, "unique");     mpack_write_bool(w, e->unique);
    mpack_write_cstr(w, "solid");      mpack_write_bool(w, e->is_solid);
    mpack_write_cstr(w, "health");     mpack_write_i32(w, e->health);
    mpack_write_cstr(w, "damage");     mpack_write_u32(w, e->damage);
    mpack_write_cstr(w, "sprite");
    mpack_build_map(w);
    mpack_write_cstr(w, "texture");    mpack_write_u32(w, e->sp.texture_id);
    mpack_write_cstr(w, "row");        mpack_write_u32(w, e->sp.current_row);
    mpack_write_cstr(w, "speed");      mpack_write_u32(w, e->sp.frames_speed);
    mpack_write_cstr(w, "scale");      write_vec2(w, e->sp.scale);
    mpack_write_cstr(w, "frame");      write_rect(w, e->sp.frame_rec);
    mpack_write_cstr(w, "tint");
    mpack_write_u32(w, (uint32_t)e->sp.tint.r | (uint32_t)e->sp.tint.g << 8 |
                       (uint32_t)e->sp.tint.b << 16 | (uint32_t)e->sp.tint.a << 24);
    mpack_write_cstr(w, "frames");
    mpack_start_array(w, e->sp.total_rows);
    for (uint32_t i = 0; i < e->sp.total_rows; ++i) {
        if (!e->sp.frames_per_row[i] || e->sp.frames_per_row[i] > LEVEL_MAX_FRAMES) {
            return false;
        }
        mpack_write_u32(w, e->sp.frames_per_row[i]);
    }
    mpack_finish_array(w);
    mpack_complete_map(w);
    mpack_write_cstr(w, "collider");
    mpack_start_array(w, (uint32_t)arrlen(e->collider.vertices));
    for (int i = 0; i < arrlen(e->collider.vertices); ++i) {
        write_vec2(w, e->collider.vertices[i]);
    }
    mpack_finish_array(w);
    mpack_complete_map(w);
    return true;
}

static bool read_entity(mpack_node_t node, struct entity *e)
{
    uint32_t type = mpack_node_u32(mpack_node_map_cstr(node, "type"));
    if (type > ENEMY) {
        return false;
    }
    *e = entity_init(type);
    e->layer = mpack_node_u32(mpack_node_map_cstr(node, "layer"));
    if (!read_vec2(mpack_node_map_cstr(node, "position"), &e->position) ||
        !read_vec2(mpack_node_map_cstr(node, "speed"), &e->speed) ||
        !read_vec2(mpack_node_map_cstr(node, "facing"), &e->facing)) {
        return false;
    }
    e->lifetime = mpack_node_u32(mpack_node_map_cstr(node, "lifetime"));
    e->is_visible = mpack_node_bool(mpack_node_map_cstr(node, "visible"));
    e->unique = mpack_node_bool(mpack_node_map_cstr(node, "unique"));
    e->is_solid = mpack_node_bool(mpack_node_map_cstr(node, "solid"));
    e->health = mpack_node_i32(mpack_node_map_cstr(node, "health"));
    e->damage = mpack_node_u32(mpack_node_map_cstr(node, "damage"));
    mpack_node_t sprite = mpack_node_map_cstr(node, "sprite");
    uint32_t texture = mpack_node_u32(mpack_node_map_cstr(sprite, "texture"));
    uint32_t row = mpack_node_u32(mpack_node_map_cstr(sprite, "row"));
    uint32_t speed = mpack_node_u32(mpack_node_map_cstr(sprite, "speed"));
    Vector2 scale;
    Rectangle frame;
    if (!read_vec2(mpack_node_map_cstr(sprite, "scale"), &scale) ||
        !read_rect(mpack_node_map_cstr(sprite, "frame"), &frame)) {
        return false;
    }
    uint32_t tint = mpack_node_u32(mpack_node_map_cstr(sprite, "tint"));
    mpack_node_t frames = mpack_node_map_cstr(sprite, "frames");
    size_t rows = mpack_node_array_length(frames);
    if (texture >= ASSETS_MAX_TEXTURES || rows > LEVEL_MAX_ROWS || (rows && row >= rows)) {
        return false;
    }
    if (rows) {
        uint32_t counts[LEVEL_MAX_ROWS];
        for (size_t i = 0; i < rows; ++i) {
            counts[i] = mpack_node_u32(mpack_node_array_at(frames, i));
            if (!counts[i] || counts[i] > LEVEL_MAX_FRAMES) {
                return false;
            }
        }
        e->sp = sprite_init(texture, (uint32_t)rows, counts, speed);
        if (!e->sp.frames_per_row) {
            return false;
        }
    }
    e->sp.texture_id = texture;
    e->sp.current_row = row;
    e->sp.frames_speed = speed;
    e->sp.scale = scale;
    e->sp.frame_rec = frame;
    e->sp.tint = (Color){tint, tint >> 8, tint >> 16, tint >> 24};
    mpack_node_t collider = mpack_node_map_cstr(node, "collider");
    size_t count = mpack_node_array_length(collider);
    if (count > LEVEL_MAX_POLYGON_VERTICES || (count && count < 3)) {
        return false;
    }
    if (count) {
        arrsetlen(e->collider.vertices, count);
        for (size_t i = 0; i < count; ++i) {
            if (!read_vec2(mpack_node_array_at(collider, i), &e->collider.vertices[i])) {
                return false;
            }
        }
    }
    return true;
}

bool level_init(struct level *level, uint32_t width, uint32_t height, uint32_t tile_size)
{
    if (!level || !width || !height || !tile_size ||
        (uint64_t)width * height > LEVEL_MAX_CELLS ||
        tile_size > UINT32_MAX / width || tile_size > UINT32_MAX / height) {
            return false;
        }
    *level = (struct level){.width = width, .height = height, .tile_size = tile_size};
    return true;
}

bool level_add_layer(struct level *level, uint32_t type)
{
    if (!level || !level->width || type >= LEVEL_LAYER_COUNT || arrlen(level->layers) >= LEVEL_MAX_LAYERS) {
        return false;
    }
    arrput(level->layers, ((struct level_layer){.type = type, .visible = 1}));
    return true;
}

int level_add_entity(struct level *level, struct entity entity)
{
    if (!level || !entity.is_active || entity.layer >= arrlenu(level->layers) ||
        arrlen(level->entities) >= LEVEL_MAX_ENTITIES) {
        return -1;
    }
    for (int i = 0; i < arrlen(level->entities); ++i) {
        if (level->entities[i].type == entity.type) {
            if (entity.unique) {
                for (int j = i + 1; j < arrlen(level->entities); ++j) {
                    if (level->entities[j].type == entity.type) {
                        return -1;
                    }
                }
                if (level->next_entity_id == UINT64_MAX) {
                    return -1;
                }
                entity_free(&level->entities[i]);
                entity.id = ++level->next_entity_id;
                level->entities[i] = entity;
                return i;
            }
            if (level->entities[i].unique) {
                return -1;
            }
        }
    }
    if (level->next_entity_id == UINT64_MAX) {
        return -1;
    }
    entity.id = ++level->next_entity_id;
    arrput(level->entities, entity);
    return (int)arrlen(level->entities) - 1;
}

struct level *level_from_entity(const struct entity *entity)
{
    for (int i = 0; i < arrlen(gs.levels); ++i) {
        struct level *level = &gs.levels[i];
        for (int j = 0; j < arrlen(level->entities); ++j) {
            if (&level->entities[j] == entity) {
                return level;
            }
        }
    }
    return NULL;
}

void level_free(struct level *level)
{
    if (!level) {
        return;
    }
    for (int i = 0; i < arrlen(level->entities); ++i) {
        entity_free(&level->entities[i]);
    }
    arrfree(level->layers);
    arrfree(level->entities);
    *level = (struct level){0};
}

bool level_save(const struct level *levels, const char *path)
{
    if (!path || !arrlen(levels) || arrlen(levels) > LEVEL_MAX_LEVELS) {
        return false;
    }
    char *data = NULL;
    size_t size = 0;
    mpack_writer_t w;
    mpack_writer_init_growable(&w, &data, &size);
    bool ok = true;
    mpack_start_array(&w, (uint32_t)arrlen(levels));
    for (int n = 0; ok && n < arrlen(levels); ++n) {
        const struct level *level = &levels[n];
        if (!level->width || !arrlen(level->layers) ||
            arrlen(level->layers) > LEVEL_MAX_LAYERS || arrlen(level->entities) > LEVEL_MAX_ENTITIES) {
            ok = false;
            break;
        }
        mpack_build_map(&w);
        mpack_write_cstr(&w, "width");      mpack_write_u32(&w, level->width);
        mpack_write_cstr(&w, "height");     mpack_write_u32(&w, level->height);
        mpack_write_cstr(&w, "tile_size");  mpack_write_u32(&w, level->tile_size);
        mpack_write_cstr(&w, "layers");
        mpack_start_array(&w, (uint32_t)arrlen(level->layers));
        for (int i = 0; i < arrlen(level->layers); ++i) {
            mpack_start_array(&w, 2);
            mpack_write_u32(&w, level->layers[i].type);
            mpack_write_bool(&w, level->layers[i].visible);
            mpack_finish_array(&w);
        }
        mpack_finish_array(&w);
        mpack_write_cstr(&w, "entities");
        mpack_start_array(&w, (uint32_t)arrlen(level->entities));
        for (int i = 0; ok && i < arrlen(level->entities); ++i) {
            if (level->entities[i].layer >= arrlenu(level->layers)) {
                ok = false;
                break;
            }
            ok = write_entity(&w, &level->entities[i]);
        }
        if (ok) {
            mpack_finish_array(&w);
            mpack_complete_map(&w);
        }
    }
    if (ok) {
        mpack_finish_array(&w);
    }
    if (!ok) {
        mpack_writer_flag_error(&w, mpack_error_data);
    }
    if (mpack_writer_destroy(&w) != mpack_ok || size > UINT32_MAX || size > LEVEL_MAX_FILE) {
        ok = false;
    }
    if (ok) {
        FILE *f = fopen(path, "wb");
        if (!f) {
            ok = false;
        } else {
            ok = fwrite("RLCM", 1, 4, f) == 4 && put32(f, LEVEL_VERSION) &&
                 chunk(f, "LVLS", (uint32_t)size) && fwrite(data, 1, size, f) == size &&
                 assets_save(f);
            if (fclose(f)) {
                ok = false;
            }
        }
    }
    free(data);
    return ok;
}

static bool read_levels(const char *data, size_t size, struct level **loaded)
{
    mpack_tree_t tree;
    mpack_tree_init_data(&tree, data, size);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);
    size_t n = mpack_node_array_length(root);
    bool ok = n && n <= LEVEL_MAX_LEVELS;
    for (size_t i = 0; ok && i < n; ++i) {
        mpack_node_t node = mpack_node_array_at(root, i);
        uint32_t width = mpack_node_u32(mpack_node_map_cstr(node, "width"));
        uint32_t height = mpack_node_u32(mpack_node_map_cstr(node, "height"));
        uint32_t tile = mpack_node_u32(mpack_node_map_cstr(node, "tile_size"));
        struct level level = {0};
        ok = level_init(&level, width, height, tile);
        if (!ok) {
            break;
        }
        arrput(*loaded, level);
        struct level *current = &arrlast(*loaded);
        mpack_node_t layers = mpack_node_map_cstr(node, "layers");
        size_t lc = mpack_node_array_length(layers);
        ok = lc && lc <= LEVEL_MAX_LAYERS;
        for (size_t j = 0; ok && j < lc; ++j) {
            mpack_node_t pair = mpack_node_array_at(layers, j);
            if (mpack_node_array_length(pair) != 2) {
                ok = false;
                break;
            }
            uint32_t type = mpack_node_u32(mpack_node_array_at(pair, 0));
            bool visible = mpack_node_bool(mpack_node_array_at(pair, 1));
            ok = level_add_layer(current, type);
            if (ok) {
                arrlast(current->layers).visible = visible;
            }
        }
        mpack_node_t entities = mpack_node_map_cstr(node, "entities");
        size_t ec = mpack_node_array_length(entities);
        if (ec > LEVEL_MAX_ENTITIES) {
            ok = false;
        }
        for (size_t j = 0; ok && j < ec; ++j) {
            struct entity e = {0};
            ok = read_entity(mpack_node_array_at(entities, j), &e) &&
                 e.layer < arrlenu(current->layers);
            for (int k = 0; ok && k < arrlen(current->entities); ++k) {
                if (current->entities[k].type == e.type &&
                    (e.unique || current->entities[k].unique)) {
                    ok = false;
                }
            }
            if (ok) {
                e.id = ++current->next_entity_id;
                arrput(current->entities, e);
            } else {
                entity_free(&e);
            }
        }
        for (int j = 0; ok && j < arrlen(current->entities); ++j) {
            struct entity *e = &current->entities[j];
            if (e->is_solid && !arrlen(e->collider.vertices)) {
                e->collider = physics_polygon_rectangle((Rectangle){0, 0, tile, tile});
            }
        }
    }
    if (mpack_tree_destroy(&tree) != mpack_ok) {
        ok = false;
    }
    return ok;
}

bool level_load(struct level **levels, const char *path)
{
    if (!levels || !path) {
        return false;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    struct level *loaded = NULL;
    bool ok = false, seen = false;
    uint32_t version, size;
    char tag[4];
    if (fseek(f, 0, SEEK_END)) {
        goto done;
    }
    long length = ftell(f);
    if (length < 8 || length > (long)LEVEL_MAX_FILE || fseek(f, 0, SEEK_SET)) {
        goto done;
    }
    if (fread(tag, 1, 4, f) != 4 || memcmp(tag, "RLCM", 4) ||
        !get32(f, &version) || version != LEVEL_VERSION) {
        goto done;
    }
    while (ftell(f) < length) {
        if (length - ftell(f) < 8 || fread(tag, 1, 4, f) != 4 || !get32(f, &size)) {
            goto done;
        }
        long start = ftell(f);
        if (start < 0 || (uint64_t)start + size > (uint64_t)length) {
            goto done;
        }
        if (!memcmp(tag, "LVLS", 4)) {
            if (seen || !size) {
                goto done;
            }
            char *data = malloc(size);
            if (!data) {
                goto done;
            }
            bool parsed = fread(data, 1, size, f) == size && read_levels(data, size, &loaded);
            free(data);
            if (!parsed) {
                goto done;
            }
            seen = true;
        } else if (!memcmp(tag, "GFX ", 4)) {
            if (!seen || !assets_read(f, size)) {
                goto done;
            }
        } else {
            goto done;
        }
        if (fseek(f, start + size, SEEK_SET)) {
            goto done;
        }
    }
    if (!seen) {
        goto done;
    }
    for (int i = 0; i < arrlen(*levels); ++i) {
        level_free(&(*levels)[i]);
    }
    arrfree(*levels);
    *levels = loaded;
    loaded = NULL;
    ok = true;
done:
    for (int i = 0; i < arrlen(loaded); ++i) {
        level_free(&loaded[i]);
    }
    arrfree(loaded);
    fclose(f);
    return ok;
}
