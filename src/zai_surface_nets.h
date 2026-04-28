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

    for (i = 0; i < dim * dim2; i++)
    {
        ctx->buffer_indices[i] = -1;
    }

    for (z = 1; z < dim - 1; ++z)
    {
        for (y = 1; y < dim - 1; ++y)
        {
            i32 row_idx = (z * dim2) + (y * dim);
            f32 d_cache[4];
            d_cache[0] = ctx->density_grid[row_idx];
            d_cache[1] = ctx->density_grid[row_idx + dim];
            d_cache[2] = ctx->density_grid[row_idx + dim2];
            d_cache[3] = ctx->density_grid[row_idx + dim2 + dim];

            for (x = 1; x < dim - 1; ++x)
            {
                i32 curr = row_idx + x;
                i32 mask = 0;
                f32 d[8];

                i32 intersections = 0;
                zai_vec3 avg_pos;

                /* Slide cache: left face comes from previous iteration */
                d[0] = d_cache[0];
                d[2] = d_cache[1];
                d[4] = d_cache[2];
                d[6] = d_cache[3];

                /* Load new right face (4 reads instead of 8) */
                d_cache[0] = ctx->density_grid[curr + 1];
                d_cache[1] = ctx->density_grid[curr + 1 + dim];
                d_cache[2] = ctx->density_grid[curr + 1 + dim2];
                d_cache[3] = ctx->density_grid[curr + 1 + dim2 + dim];

                d[1] = d_cache[0];
                d[3] = d_cache[1];
                d[5] = d_cache[2];
                d[7] = d_cache[3];

                mask =
                    ((d[0] < iso)) |
                    ((d[1] < iso) << 1) |
                    ((d[2] < iso) << 2) |
                    ((d[3] < iso) << 3) |
                    ((d[4] < iso) << 4) |
                    ((d[5] < iso) << 5) |
                    ((d[6] < iso) << 6) |
                    ((d[7] < iso) << 7);

                if (mask == 0 || mask == 255)
                {
                    ctx->buffer_indices[curr] = -1;
                    continue;
                }

                /* Vertex generation */
                avg_pos.x = avg_pos.y = avg_pos.z = 0.0f;

                for (i = 0; i < 12; i++)
                {
                    i32 i1 = edge_map[i][0];
                    i32 i2 = edge_map[i][1];

                    if (((mask >> i1) & 1) != ((mask >> i2) & 1))
                    {
                        f32 denom = d[i2] - d[i1];
                        f32 t;

                        if (zai_absf(denom) < 1e-6f)
                        {
                            continue;
                        }

                        t = (iso - d[i1]) / denom;

                        avg_pos.x += (f32)x + (f32)((i1 >> 0) & 1) + t * (f32)(((i2 >> 0) & 1) - ((i1 >> 0) & 1));
                        avg_pos.y += (f32)y + (f32)((i1 >> 1) & 1) + t * (f32)(((i2 >> 1) & 1) - ((i1 >> 1) & 1));
                        avg_pos.z += (f32)z + (f32)((i1 >> 2) & 1) + t * (f32)(((i2 >> 2) & 1) - ((i1 >> 2) & 1));

                        intersections++;
                    }
                }

                if (intersections > 0)
                {
                    f32 inv = 1.0f / (f32)intersections;
                    f32 sum_lo, sum_hi, sum_x0, sum_y0, total;
                    zai_vec3 n;
                    f32 mag_sq;

                    out_vertices[v_count].position.x = avg_pos.x * inv * scale - offset + ctx->chunk_coord.x;
                    out_vertices[v_count].position.y = avg_pos.y * inv * scale - offset + ctx->chunk_coord.y;
                    out_vertices[v_count].position.z = avg_pos.z * inv * scale - offset + ctx->chunk_coord.z;

                    /* Normal via reused partial sums */
                    sum_lo = d[0] + d[1] + d[2] + d[3];
                    sum_hi = d[4] + d[5] + d[6] + d[7];
                    sum_x0 = d[0] + d[2] + d[4] + d[6];
                    sum_y0 = d[0] + d[1] + d[4] + d[5];
                    total = sum_lo + sum_hi;

                    n.x = sum_x0 - (total - sum_x0);
                    n.y = sum_y0 - (total - sum_y0);
                    n.z = sum_lo - sum_hi;

                    mag_sq = n.x * n.x + n.y * n.y + n.z * n.z;
                    if (mag_sq > 1e-6f)
                    {
                        f32 inv_mag = zai_invsqrtf(mag_sq);
                        out_vertices[v_count].normal.x = n.x * inv_mag;
                        out_vertices[v_count].normal.y = n.y * inv_mag;
                        out_vertices[v_count].normal.z = n.z * inv_mag;
                    }
                    else
                    {
                        out_vertices[v_count].normal = zai_vec3_init(0, 1, 0);
                    }

                    ctx->buffer_indices[curr] = v_count;
                    v_count++;
                }
                else
                {
                    ctx->buffer_indices[curr] = -1;
                }

                /* Indices generation */
                {
                    i32 d_below = (ctx->density_grid[curr] < iso);
                    i32 v0, v1, v2, v3;

                    /* X-axis edge: curr <-> curr+1 */
                    if (d_below != (ctx->density_grid[curr + 1] < iso))
                    {
                        v0 = ctx->buffer_indices[curr];
                        v1 = ctx->buffer_indices[curr - dim];
                        v2 = ctx->buffer_indices[curr - dim - dim2];
                        v3 = ctx->buffer_indices[curr - dim2];

                        if (v0 >= 0 && v1 >= 0 && v2 >= 0 && v3 >= 0)
                        {
                            if (d_below)
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

                    /* Y-axis edge: curr <-> curr+dim */
                    if (d_below != (ctx->density_grid[curr + dim] < iso))
                    {
                        v0 = ctx->buffer_indices[curr];
                        v1 = ctx->buffer_indices[curr - 1];
                        v2 = ctx->buffer_indices[curr - 1 - dim2];
                        v3 = ctx->buffer_indices[curr - dim2];

                        if (v0 >= 0 && v1 >= 0 && v2 >= 0 && v3 >= 0)
                        {
                            if (d_below)
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

                    /* Z-axis edge: curr <-> curr+dim2 */
                    if (d_below != (ctx->density_grid[curr + dim2] < iso))
                    {
                        v0 = ctx->buffer_indices[curr];
                        v1 = ctx->buffer_indices[curr - 1];
                        v2 = ctx->buffer_indices[curr - 1 - dim];
                        v3 = ctx->buffer_indices[curr - dim];

                        if (v0 >= 0 && v1 >= 0 && v2 >= 0 && v3 >= 0)
                        {
                            if (d_below)
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
    }

    *out_vertex_count = v_count;
    *out_index_count = idx_count;
}

#endif /* ZAI_SURFACE_NETS_H */