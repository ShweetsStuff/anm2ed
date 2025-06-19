#pragma once

#include "PACKED.h"
#include "texture.h"

struct Packed
{
    Texture textures[PACKED_TEXTURE_COUNT];
};

void packed_init(Packed* self);
void packed_free(Packed* self);