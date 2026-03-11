#ifndef ZAI_SPARSE_GRID_H
#define ZAI_SPARSE_GRID_H

#include "zai_math_linear_algebra.h"

/* #############################################################################
 * # [SECTION] Sparse Grid Helper Functions
 * #############################################################################
 */
ZAI_API ZAI_INLINE u8 zai_types_f32_to_u8(f32 value)
{
    value = (value < 0.0f) ? 0.0f : value;
    value = (value > 255.0f) ? 255.0f : value;

    return (u8)value;
}

ZAI_API ZAI_INLINE s8 zai_types_f32_to_s8(f32 value)
{
    value = (value < -127.0f) ? -127.0f : value;
    value = (value > 127.0f) ? 127.0f : value;

    return (s8)value;
}

/* #############################################################################
 * # [SECTION] Sparse Grid Setup
 * #############################################################################
 */
#define ZAI_BRICK_SIZE 8
#define ZAI_BRICK_APRON 1
#define ZAI_PHYSICAL_BRICK_SIZE (ZAI_BRICK_SIZE + (2 * ZAI_BRICK_APRON))                                        /* 10 */
#define ZAI_BRICK_TOTAL_VOXELS (ZAI_PHYSICAL_BRICK_SIZE * ZAI_PHYSICAL_BRICK_SIZE * ZAI_PHYSICAL_BRICK_SIZE) /* 1000 */

#define ZAI_BRICK_MAP_INDEX_AIR 0         /* EMPTY (far outside): Skip Atlas Data */
#define ZAI_BRICK_MAP_INDEX_SOLID 0xFFFF  /* SOLID (far inside): Skip Atlas Data */
#define ZAI_BRICK_MAP_INDEX_USEFUL 0xFFFE /* USABLE: Sentinal for useable brick. Use this for Atlas Data */

typedef struct zai_grid_data
{
    f32 distance;
    u8 material;

} zai_grid_data;

typedef zai_grid_data (*zai_grid_distance_function)(zai_vec3 position, void *user_data);

typedef struct zai_sparse_grid
{
    /* First Pass: Evaluate Brick Map */
    u32 brick_map_dimensions;
    u32 brick_map_bytes;
    u32 brick_map_active_bricks_count;
    u16 *brick_map_data;

    /* Second Pass: Fill Atlas */
    u32 atlas_bricks_per_row;
    zai_vec3 atlas_dimensions;
    zai_vec3 atlas_dimensions_inverse;
    u32 atlas_bytes;
    s8 *atlas_data;

    u8 *material_data;

    /* Data for shader upload */
    zai_vec3 start;
    f32 cell_size;
    f32 truncation_distance;

    /* Internal: General data */
    zai_vec3 center;
    u32 cell_count;
    f32 brick_radius;

} zai_sparse_grid;

ZAI_API u8 zai_sparse_grid_initialize(zai_sparse_grid *grid, zai_vec3 grid_center, u32 grid_cell_count, f32 grid_cell_size)
{
    /* First Pass: Calculate Brick Map Memory Requirements */
    grid->brick_map_dimensions = grid_cell_count / ZAI_BRICK_SIZE;
    grid->brick_map_bytes = grid->brick_map_dimensions * grid->brick_map_dimensions * grid->brick_map_dimensions * sizeof(u16);

    /* Data for shader upload */
    grid->start = zai_vec3_subf(grid_center, (f32)grid_cell_count * grid_cell_size * 0.5f);
    grid->cell_size = grid_cell_size;
    grid->truncation_distance = grid_cell_size * 4.0f;

    /* Internal helpers */
    grid->center = grid_center;
    grid->cell_count = grid_cell_count;
    grid->brick_radius = 0.5f * ZAI_SQRT3 * (ZAI_BRICK_SIZE * grid->cell_size);

    return 1;
}

ZAI_API ZAI_INLINE u8 zai_sparse_grid_pass_01_fill_brick_map(zai_sparse_grid *grid, zai_grid_distance_function distance_function, void *user_data)
{
    u32 brick_map_index = 0;
    u32 active_brick_count = 0;

    u32 bx, by, bz;
    f32 px, py, pz;

    zai_vec3 start = grid->start;

    f32 brick_step = (f32)ZAI_BRICK_SIZE * grid->cell_size;
    f32 center_off = brick_step * 0.5f;

    f32 brick_radius = grid->brick_radius;
    f32 distance_truncation = grid->truncation_distance;

    pz = start.z;

    for (bz = 0; bz < grid->brick_map_dimensions; ++bz, pz += brick_step)
    {
        py = start.y;

        for (by = 0; by < grid->brick_map_dimensions; ++by, py += brick_step)
        {
            px = start.x;

            for (bx = 0; bx < grid->brick_map_dimensions; ++bx, px += brick_step, ++brick_map_index)
            {
                zai_vec3 center = zai_vec3_init(px + center_off, py + center_off, pz + center_off);
                zai_grid_data data = distance_function(center, user_data);

                f32 distance = data.distance;

                /* Culling */
                if ((distance - brick_radius) > distance_truncation)
                {
                    grid->brick_map_data[brick_map_index] = ZAI_BRICK_MAP_INDEX_AIR;
                }
                else if ((distance + brick_radius) < -distance_truncation)
                {
                    grid->brick_map_data[brick_map_index] = ZAI_BRICK_MAP_INDEX_SOLID;
                }
                else
                {
                    grid->brick_map_data[brick_map_index] = ZAI_BRICK_MAP_INDEX_USEFUL;
                    active_brick_count++;
                }
            }
        }
    }

    grid->brick_map_active_bricks_count = active_brick_count;

    /* Calculate atlas dimensions and bytes required (e.g. how big does the atlas 3d texture needs to be to fit all relevant bricks) */
    {
        u32 bricks_per_row = (u32)zai_ceilf(zai_sqrtf((f32)active_brick_count));
        u32 bricks_per_col = (active_brick_count + bricks_per_row - 1) / bricks_per_row;

        grid->atlas_bricks_per_row = bricks_per_row;

        grid->atlas_dimensions = zai_vec3_init(
            (f32)(bricks_per_row * ZAI_PHYSICAL_BRICK_SIZE),
            (f32)(bricks_per_col * ZAI_PHYSICAL_BRICK_SIZE),
            (f32)ZAI_PHYSICAL_BRICK_SIZE);

        grid->atlas_dimensions_inverse = zai_vec3_init(
            1.0f / (f32)grid->atlas_dimensions.x,
            1.0f / (f32)grid->atlas_dimensions.y,
            1.0f / (f32)grid->atlas_dimensions.z);

        grid->atlas_bytes = (u32)(grid->atlas_dimensions.x * grid->atlas_dimensions.y * grid->atlas_dimensions.z) * sizeof(u8);
    }

    return 1;
}

ZAI_API ZAI_INLINE u8 zai_sparse_grid_pass_02_fill_atlas(zai_sparse_grid *grid, zai_grid_distance_function distance_function, void *user_data)
{
    u32 bricks_per_row = grid->atlas_bricks_per_row;
    u32 atlas_used_count = 0;
    f32 quant_scale = 127.0f / grid->truncation_distance;

    f32 apron_offset = -((f32)ZAI_BRICK_APRON * grid->cell_size);
    u32 atlas_width = bricks_per_row * ZAI_PHYSICAL_BRICK_SIZE;
    u32 atlas_height = ((grid->brick_map_active_bricks_count + bricks_per_row - 1) / bricks_per_row) * ZAI_PHYSICAL_BRICK_SIZE;

    u32 atlas_vox_stride = atlas_width;
    u32 atlas_slice_stride = atlas_width * atlas_height;

    f32 brick_step = ZAI_BRICK_SIZE * grid->cell_size;

    u32 bx, by, bz;
    u32 lx, ly, lz;

    for (bz = 0; bz < grid->brick_map_dimensions; ++bz)
    {
        for (by = 0; by < grid->brick_map_dimensions; ++by)
        {
            for (bx = 0; bx < grid->brick_map_dimensions; ++bx)
            {
                u32 map_idx = bx + (by * grid->brick_map_dimensions) + (bz * grid->brick_map_dimensions * grid->brick_map_dimensions);
                u32 cur_idx;
                u32 atlas_bx;
                u32 atlas_by;

                zai_vec3 physical_position;

                if (grid->brick_map_data[map_idx] != ZAI_BRICK_MAP_INDEX_USEFUL)
                {
                    continue;
                }

                /* 1. Determine Atlas Destination (Brick Coordinates) */
                cur_idx = atlas_used_count++;
                atlas_bx = cur_idx % bricks_per_row;
                atlas_by = cur_idx / bricks_per_row;

                /* 3. Voxel Fill Loop */
                physical_position.z = (grid->start.z + (f32)(bz)*brick_step) + apron_offset;

                for (lz = 0; lz < ZAI_PHYSICAL_BRICK_SIZE; ++lz, physical_position.z += grid->cell_size)
                {
                    physical_position.y = (grid->start.y + (f32)(by)*brick_step) + apron_offset;

                    for (ly = 0; ly < ZAI_PHYSICAL_BRICK_SIZE; ++ly, physical_position.y += grid->cell_size)
                    {
                        /* Calculate destination pointer for this row (x-line) in the atlas */
                        u32 dst_x = atlas_bx * ZAI_PHYSICAL_BRICK_SIZE;
                        u32 dst_y = (atlas_by * ZAI_PHYSICAL_BRICK_SIZE) + ly;
                        u32 dst_z = lz;

                        s8 *dst_row = &grid->atlas_data[dst_x + (dst_y * atlas_vox_stride) + (dst_z * atlas_slice_stride)];
                        u8 *dst_material_row = &grid->material_data[dst_x + (dst_y * atlas_vox_stride) + (dst_z * atlas_slice_stride)];

                        physical_position.x = (grid->start.x + (f32)(bx)*brick_step) + apron_offset;

                        for (lx = 0; lx < ZAI_PHYSICAL_BRICK_SIZE; ++lx, physical_position.x += grid->cell_size)
                        {
                            zai_grid_data data = distance_function(physical_position, user_data);

                            /* Quantize: map [-trunc, +trunc] to [-127, 127] */
                            f32 val = data.distance * quant_scale;
                            dst_row[lx] = zai_types_f32_to_s8(val);

                            dst_material_row[lx] = data.material;
                        }
                    }
                }

                /* 4. Update Map with 1-based index to Atlas Brick */
                grid->brick_map_data[map_idx] = (u16)(cur_idx + 1);
            }
        }
    }

    return 1;
}

#endif /* ZAI_SPARSE_GRID_H */