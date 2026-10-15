#ifndef ASSETS_H
#define ASSETS_H

#include <raylib.h>
#include <stdio.h>
#include <stdint.h>
#include "stb_ds.h"

#define ASSETS_DIR "graphics"
#define ASSETS_FILE "graphics/data.bin"
#define ASSETS_PLAYER_PATH "graphics/player.png"
#define ASSETS_FALLBACK_PATH "graphics/no_texture.png"
#define ASSETS_TILESET_PATH "graphics/tileset.png"
#define ASSETS_BULLET_PATH "graphics/bullet.png"
#define ASSETS_IMAGE_FORMAT ".png"
#define ASSETS_MAX_TEXTURES 4096u
#define ASSETS_MAX_IMAGE (32u * 1024u * 1024u)

enum {
    TEX_PLAYER,
    TEX_TILESET,
    TEX_NO_TEXTURE,
    TEX_BULLET,
    TEX_COUNT
};

struct asset_image {
    uint32_t id;
    unsigned char *data;
};

void assets_load(void);
void assets_free(void);
Texture2D assets_texture(uint32_t id);
bool assets_import(uint32_t id, const char *path);
bool assets_save(FILE *f);
bool assets_read(FILE *f, uint32_t size);

#ifdef ASSETS_IMPLEMENTATION

static Texture2D *textures;
static struct asset_image *images;

void assets_load(void)
{
    size_t old = arrlenu(textures);
    if (old < TEX_COUNT) {
        arrsetlen(textures, TEX_COUNT);
        for (size_t i = old; i < TEX_COUNT; ++i) { textures[i] = (Texture2D){0}; }
    }
    if (FileExists(ASSETS_PLAYER_PATH)) { assets_import(TEX_PLAYER, ASSETS_PLAYER_PATH); }
    if (FileExists(ASSETS_TILESET_PATH)) { assets_import(TEX_TILESET, ASSETS_TILESET_PATH); }
    if (FileExists(ASSETS_FALLBACK_PATH)) { assets_import(TEX_NO_TEXTURE, ASSETS_FALLBACK_PATH); }
    if (FileExists(ASSETS_BULLET_PATH)) { assets_import(TEX_BULLET, ASSETS_BULLET_PATH); }
}

void assets_free(void)
{
    for (int i = 0; i < arrlen(textures); ++i) {
        if (textures[i].id) { UnloadTexture(textures[i]); }
    }
    for (int i = 0; i < arrlen(images); ++i) { arrfree(images[i].data); }
    arrfree(images);
    arrfree(textures);
}

Texture2D assets_texture(uint32_t id)
{
    return id < arrlenu(textures) ? textures[id] : (Texture2D){0};
}

static bool assets_store(struct asset_image img)
{
    Image decoded = LoadImageFromMemory(ASSETS_IMAGE_FORMAT, img.data, (int)arrlen(img.data));
    if (!decoded.data) { return false; }
    Texture2D texture = LoadTextureFromImage(decoded);
    UnloadImage(decoded);
    if (!texture.id) { return false; }
    if (img.id >= arrlenu(textures)) {
        size_t old = arrlenu(textures);
        arrsetlen(textures, img.id + 1);
        for (size_t i = old; i < arrlenu(textures); ++i) { textures[i] = (Texture2D){0}; }
    }
    if (textures[img.id].id) { UnloadTexture(textures[img.id]); }
    textures[img.id] = texture;
    for (int i = 0; i < arrlen(images); ++i) {
        if (images[i].id == img.id) {
            arrfree(images[i].data);
            images[i] = img;
            return true;
        }
    }
    arrput(images, img);
    return true;
}

bool assets_read(FILE *f, uint32_t size)
{
    if (size < 4 || size > ASSETS_MAX_IMAGE + 4) { return false; }
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) { return false; }
    struct asset_image img = {.id = (uint32_t)b[0] | (uint32_t)b[1] << 8 |
                                    (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24};
    if (img.id >= ASSETS_MAX_TEXTURES) { return false; }
    arrsetlen(img.data, size - 4);
    bool ok = fread(img.data, 1, arrlen(img.data), f) == arrlenu(img.data) && assets_store(img);
    if (!ok) { arrfree(img.data); }
    return ok;
}

bool assets_import(uint32_t id, const char *path)
{
    if (id >= ASSETS_MAX_TEXTURES || !path) { return false; }
    FILE *f = fopen(path, "rb");
    if (!f) { return false; }
    bool ok = false;
    if (!fseek(f, 0, SEEK_END)) {
        long size = ftell(f);
        if (size > 0 && size <= (long)ASSETS_MAX_IMAGE && !fseek(f, 0, SEEK_SET)) {
            struct asset_image img = {.id = id};
            arrsetlen(img.data, (size_t)size);
            ok = fread(img.data, 1, (size_t)size, f) == (size_t)size && assets_store(img);
            if (!ok) { arrfree(img.data); }
        }
    }
    fclose(f);
    return ok;
}

bool assets_save(FILE *f)
{
    for (int i = 0; i < arrlen(images); ++i) {
        struct asset_image *img = &images[i];
        uint32_t size = (uint32_t)arrlen(img->data) + 4;
        unsigned char header[12] = {'G','F','X',' ', size, size >> 8, size >> 16, size >> 24,
                                    img->id, img->id >> 8, img->id >> 16, img->id >> 24};
        if (fwrite(header, 1, sizeof(header), f) != sizeof(header) ||
            fwrite(img->data, 1, arrlen(img->data), f) != arrlenu(img->data)) { return false; }
    }
    return true;
}

#endif
#endif //ASSETS_H
