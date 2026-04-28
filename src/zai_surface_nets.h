#ifndef ZAI_SURFACE_NETS_H
#define ZAI_SURFACE_NETS_H

#include "zai_types.h"
#include "zai_math_linear_algebra.h"

/* #############################################################################
 * # [SECTION] Surface Nets
 * #############################################################################
 */
typedef struct zai_surface_nets_vertex
{
    zai_vec3 position;
    zai_vec3 normal;
} zai_surface_nets_vertex;

typedef struct zai_surface_nets_context
{
    f32 *density_grid;    /* 1D array [z][y][x] */
    i32 *buffer_indices;  /* 1D array [z][y][x], size dim_size^3 */
    i32 dim_size;         /* Points per axis */
    f32 grid_size;        /* World space size */
    f32 iso_level;        /* Surface threshold */
    zai_vec3 chunk_coord; /* World offset */
} zai_surface_nets_context;

ZAI_API ZAI_INLINE void zai_surface_nets_generate(
    zai_surface_nets_context *ctx,
    zai_surface_nets_vertex *out_vertices,
    i32 *out_vertex_count,
    u32 *out_indices,
    i32 *out_index_count)
{
    i32 x, y, z, i;
    i32 v_count = 0;
    i32 idx_count = 0;
    i32 dim = ctx->dim_size;
    i32 dim2 = dim * dim;
    f32 scale = ctx->grid_size / (f32)(dim - 1);
    f32 offset = ctx->grid_size * 0.5f;
    f32 iso = ctx->iso_level;

    static i32 edge_map[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

    /* Pass 1: Generate Vertices */
    for (z = 0; z < dim - 1; ++z)
    {
        for (y = 0; y < dim - 1; ++y)
        {
            i32 row_idx = (z * dim2) + (y * dim);

            for (x = 0; x < dim - 1; x++)
            {
                i32 curr_cell = row_idx + x;
                i32 mask = 0;
                f32 d[8];
                i32 intersections = 0;
                zai_vec3 avg_pos;

                avg_pos.x = 0.0f;
                avg_pos.y = 0.0f;
                avg_pos.z = 0.0f;

                d[0] = ctx->density_grid[curr_cell];
                d[1] = ctx->density_grid[curr_cell + 1];
                d[2] = ctx->density_grid[curr_cell + dim];
                d[3] = ctx->density_grid[curr_cell + dim + 1];
                d[4] = ctx->density_grid[curr_cell + dim2];
                d[5] = ctx->density_grid[curr_cell + dim2 + 1];
                d[6] = ctx->density_grid[curr_cell + dim2 + dim];
                d[7] = ctx->density_grid[curr_cell + dim2 + dim + 1];

                mask =
                    (d[0] < iso) |
                    ((d[1] < iso) << 1) |
                    ((d[2] < iso) << 2) |
                    ((d[3] < iso) << 3) |
                    ((d[4] < iso) << 4) |
                    ((d[5] < iso) << 5) |
                    ((d[6] < iso) << 6) |
                    ((d[7] < iso) << 7);

                if (mask == 0 || mask == 255)
                {
                    ctx->buffer_indices[curr_cell] = -1;
                    continue;
                }

                for (i = 0; i < 12; i++)
                {
                    i32 i1 = edge_map[i][0];
                    i32 i2 = edge_map[i][1];

                    if (((mask >> i1) & 1) != ((mask >> i2) & 1))
                    {
                        f32 t = (iso - d[i1]) / (d[i2] - d[i1]);

                        avg_pos.x += (f32)x + (f32)((i1 >> 0) & 1) + t * (f32)(((i2 >> 0) & 1) - ((i1 >> 0) & 1));
                        avg_pos.y += (f32)y + (f32)((i1 >> 1) & 1) + t * (f32)(((i2 >> 1) & 1) - ((i1 >> 1) & 1));
                        avg_pos.z += (f32)z + (f32)((i1 >> 2) & 1) + t * (f32)(((i2 >> 2) & 1) - ((i1 >> 2) & 1));

                        intersections++;
                    }
                }

                if (intersections > 0)
                {
                    f32 inv_int = 1.0f / (f32)intersections;
                    zai_vec3 local_p;
                    zai_vec3 n;
                    f32 mag_sq;

                    local_p.x = avg_pos.x * inv_int;
                    local_p.y = avg_pos.y * inv_int;
                    local_p.z = avg_pos.z * inv_int;

                    out_vertices[v_count].position.x = local_p.x * scale - offset + ctx->chunk_coord.x;
                    out_vertices[v_count].position.y = local_p.y * scale - offset + ctx->chunk_coord.y;
                    out_vertices[v_count].position.z = local_p.z * scale - offset + ctx->chunk_coord.z;

                    /* Central Difference Gradient for Normals */
                    n.x = (d[0] + d[2] + d[4] + d[6]) - (d[1] + d[3] + d[5] + d[7]);
                    n.y = (d[0] + d[1] + d[4] + d[5]) - (d[2] + d[3] + d[6] + d[7]);
                    n.z = (d[0] + d[1] + d[2] + d[3]) - (d[4] + d[5] + d[6] + d[7]);

                    mag_sq = n.x * n.x + n.y * n.y + n.z * n.z;

                    if (mag_sq > 1e-6f)
                    {
                        f32 inv_mag = 1.0f / zai_sqrtf(mag_sq);

                        out_vertices[v_count].normal.x = n.x * inv_mag;
                        out_vertices[v_count].normal.y = n.y * inv_mag;
                        out_vertices[v_count].normal.z = n.z * inv_mag;
                    }
                    else
                    {
                        out_vertices[v_count].normal = zai_vec3_init(0, 1, 0);
                    }

                    ctx->buffer_indices[curr_cell] = v_count;
                    v_count++;
                }
            }
        }
    }

    /* Pass 2: Generate Indices (Quads) */
    for (z = 1; z < dim - 1; ++z)
    {
        for (y = 1; y < dim - 1; ++y)
        {
            i32 row_idx = (z * dim2) + (y * dim);

            for (x = 1; x < dim - 1; ++x)
            {
                i32 curr = row_idx + x;
                f32 d = ctx->density_grid[curr];
                i32 v0, v1, v2, v3;

                /* Each edge can potentially generate a quad */
                /* X-axis edge check */
                if ((d < iso) != (ctx->density_grid[curr + 1] < iso))
                {
                    v0 = ctx->buffer_indices[curr];
                    v1 = ctx->buffer_indices[curr - dim];
                    v2 = ctx->buffer_indices[curr - dim - dim2];
                    v3 = ctx->buffer_indices[curr - dim2];

                    if (v0 != -1 && v1 != -1 && v2 != -1 && v3 != -1)
                    {
                        if (d < iso)
                        {
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v2;
                            out_indices[idx_count++] = (u32)v1;
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v3;
                            out_indices[idx_count++] = (u32)v2;
                        }
                        else
                        {
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v1;
                            out_indices[idx_count++] = (u32)v2;
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v2;
                            out_indices[idx_count++] = (u32)v3;
                        }
                    }
                }

                /* Y-axis edge check */
                if ((d < iso) != (ctx->density_grid[curr + dim] < iso))
                {
                    v0 = ctx->buffer_indices[curr];
                    v1 = ctx->buffer_indices[curr - 1];
                    v2 = ctx->buffer_indices[curr - 1 - dim2];
                    v3 = ctx->buffer_indices[curr - dim2];

                    if (v0 != -1 && v1 != -1 && v2 != -1 && v3 != -1)
                    {
                        if (d < iso)
                        {
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v1;
                            out_indices[idx_count++] = (u32)v2;
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v2;
                            out_indices[idx_count++] = (u32)v3;
                        }
                        else
                        {
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v2;
                            out_indices[idx_count++] = (u32)v1;
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v3;
                            out_indices[idx_count++] = (u32)v2;
                        }
                    }
                }

                /* Z-axis edge check */
                if ((d < iso) != (ctx->density_grid[curr + dim2] < iso))
                {
                    v0 = ctx->buffer_indices[curr];
                    v1 = ctx->buffer_indices[curr - 1];
                    v2 = ctx->buffer_indices[curr - 1 - dim];
                    v3 = ctx->buffer_indices[curr - dim];

                    if (v0 != -1 && v1 != -1 && v2 != -1 && v3 != -1)
                    {
                        if (d < iso)
                        {
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v2;
                            out_indices[idx_count++] = (u32)v1;
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v3;
                            out_indices[idx_count++] = (u32)v2;
                        }
                        else
                        {
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v1;
                            out_indices[idx_count++] = (u32)v2;
                            out_indices[idx_count++] = (u32)v0;
                            out_indices[idx_count++] = (u32)v2;
                            out_indices[idx_count++] = (u32)v3;
                        }
                    }
                }
            }
        }
    }

    *out_vertex_count = v_count;
    *out_index_count = idx_count;
}

#endif /* ZAI_SURFACE_NETS_H */