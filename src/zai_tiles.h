#ifndef ZAI_TILES_H
#define ZAI_TILES_H

#include "zai_types.h"

/* #############################################################################
 * # [SECTION] Tiling Logic
 * #############################################################################
 */
#define ZAI_TILES_PER_SIDE 5
#define ZAI_TILES_TOTAL (ZAI_TILES_PER_SIDE * ZAI_TILES_PER_SIDE)

/* SoA tiles setup */
typedef struct zai_tiles
{
    /* General information */
    i32 origin_x; /* The bottom left origin X */
    i32 origin_z; /* The bottom left origin Z */

    /* Data Arrays (Indexed via toroidal wrapping) */
    i32 tile_x[ZAI_TILES_TOTAL];
    i32 tile_z[ZAI_TILES_TOTAL];

    /* Sparse Dirty flag tracking */
    u16 dirty_indices[ZAI_TILES_TOTAL]; /* Which index ot the tile_x/z is marked dirty */
    u16 dirty_indices_count;            /* Total number of dirty tile indices */

} zai_tiles;

ZAI_API ZAI_INLINE i32 zai_modi(i32 a, i32 b)
{
    i32 r = a % b;
    return r < 0 ? r + b : r;
}

ZAI_API ZAI_INLINE i32 zai_absi(i32 v)
{
    return v < 0 ? -v : v;
}

ZAI_API ZAI_INLINE u32 zai_tile_index(i32 x, i32 z)
{
    i32 slot_x = zai_modi(x, ZAI_TILES_PER_SIDE);
    i32 slot_z = zai_modi(z, ZAI_TILES_PER_SIDE);
    return (u32)(slot_z * ZAI_TILES_PER_SIDE + slot_x);
}

ZAI_API ZAI_INLINE u8 zai_tile_is_dirty(zai_tiles *t, u32 tile_index)
{
    u16 i;

    for (i = 0; i < t->dirty_indices_count; ++i)
    {
        if (t->dirty_indices[i] == tile_index)
        {
            return 1;
        }
    }

    return 0;
}

/*
| Tile | World |
| ---: | ----: |
|   -1 |  -1.5 |
|    0 |  -0.5 |
|    1 |   0.5 |
|    2 |   1.5 |
*/
ZAI_API ZAI_INLINE f32 zai_tile_to_world(i32 tile, f32 tile_size)
{
    return ((f32)tile - 0.5f) * tile_size;
}

/*
| World | Tile |
| ----: | ---: |
|  -0.5 |    0 |
|   0.0 |    0 |
|  0.49 |    0 |
|  0.50 |    1 |
|  1.49 |    1 |
|  1.50 |    2 |
*/
ZAI_API ZAI_INLINE i32 zai_world_to_tile(f32 world, f32 tile_size)
{
    f32 res = world / tile_size + 0.5f;
    i32 i = (i32)res;
    return (res < (f32)i) ? (i - 1) : i;
}

ZAI_API void zai_tiles_init(zai_tiles *t, i32 camera_tile_x, i32 camera_tile_z)
{
    i32 x, z;
    i32 half = ZAI_TILES_PER_SIDE / 2;

    t->origin_x = camera_tile_x - half;
    t->origin_z = camera_tile_z - half;
    t->dirty_indices_count = 0;

    for (z = t->origin_z; z < t->origin_z + ZAI_TILES_PER_SIDE; ++z)
    {
        for (x = t->origin_x; x < t->origin_x + ZAI_TILES_PER_SIDE; ++x)
        {
            u32 i = zai_tile_index(x, z);

            t->tile_x[i] = x;
            t->tile_z[i] = z;
            t->dirty_indices[t->dirty_indices_count++] = (u16)i;
        }
    }
}

/* Toroidal wrap around */
ZAI_API ZAI_INLINE void zai_tiles_update(zai_tiles *t, i32 camera_tile_x, i32 camera_tile_z)
{
    i32 half = ZAI_TILES_PER_SIDE / 2;
    i32 new_origin_x = camera_tile_x - half;
    i32 new_origin_z = camera_tile_z - half;
    i32 x, z;

    /* If camera is to far away reinitialize everyting */
    if (zai_absi(new_origin_x - t->origin_x) >= ZAI_TILES_PER_SIDE ||
        zai_absi(new_origin_z - t->origin_z) >= ZAI_TILES_PER_SIDE)
    {
        zai_tiles_init(t, camera_tile_x, camera_tile_z);
        return;
    }

    /* Move East */
    while (t->origin_x < new_origin_x)
    {
        i32 target_x = t->origin_x + ZAI_TILES_PER_SIDE;

        for (z = t->origin_z; z < t->origin_z + ZAI_TILES_PER_SIDE; ++z)
        {
            u32 i = zai_tile_index(target_x, z);

            t->tile_x[i] = target_x;
            t->tile_z[i] = z;
            t->dirty_indices[t->dirty_indices_count++] = (u16)i;
        }

        t->origin_x++;
    }

    /* Move West */
    while (t->origin_x > new_origin_x)
    {
        i32 target_x;

        t->origin_x--;
        target_x = t->origin_x;

        for (z = t->origin_z; z < t->origin_z + ZAI_TILES_PER_SIDE; ++z)
        {
            u32 i = zai_tile_index(target_x, z);

            t->tile_x[i] = target_x;
            t->tile_z[i] = z;
            t->dirty_indices[t->dirty_indices_count++] = (u16)i;
        }
    }

    /* Move South */
    while (t->origin_z < new_origin_z)
    {
        i32 target_z = t->origin_z + ZAI_TILES_PER_SIDE;

        for (x = t->origin_x; x < t->origin_x + ZAI_TILES_PER_SIDE; ++x)
        {
            u32 i = zai_tile_index(x, target_z);

            t->tile_x[i] = x;
            t->tile_z[i] = target_z;
            t->dirty_indices[t->dirty_indices_count++] = (u16)i;
        }

        t->origin_z++;
    }

    /* Move North */
    while (t->origin_z > new_origin_z)
    {
        i32 target_z;

        t->origin_z--;
        target_z = t->origin_z;

        for (x = t->origin_x; x < t->origin_x + ZAI_TILES_PER_SIDE; ++x)
        {
            u32 i = zai_tile_index(x, target_z);

            t->tile_x[i] = x;
            t->tile_z[i] = target_z;
            t->dirty_indices[t->dirty_indices_count++] = (u16)i;
        }
    }
}

#endif /* ZAI_TILES_H */