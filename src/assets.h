#ifndef ASSETS_H
#define ASSETS_H

#include <raylib.h>

enum {
    TEX_PLAYER,
    TEX_COUNT
};

void assets_load(void);
void assets_free(void);
Texture2D assets_texture(int id);

#ifdef ASSETS_IMPLEMENTATION

static Texture2D textures[TEX_COUNT];

void assets_load(void)
{
    textures[TEX_PLAYER] = LoadTexture("graphics/spaceship/UFO.png");
}

void assets_free(void)
{
    for (int i = 0; i < TEX_COUNT; ++i) {
        if (textures[i].id) UnloadTexture(textures[i]);
    }
}

Texture2D assets_texture(int id)
{
    return textures[id];
}

#endif
#endif //ASSETS_H
