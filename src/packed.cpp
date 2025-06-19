#include "packed.h"

/* Loads in packed data */
void
packed_init(Packed* self)
{
    for (s32 i = 0; i < PACKED_TEXTURE_COUNT; i++)
        texture_from_data_init(&self->textures[i], (u8*)PACKED_TEXTURE_DATA[i].data, PACKED_TEXTURE_DATA[i].length);
}

/* Frees packed data */
void
packed_free(Packed* self)
{
    for (s32 i = 0; i < PACKED_TEXTURE_COUNT; i++)
        texture_free(&self->textures[i]);
}