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

#define ZAI_SURFACE_NETS_INDEX(ctx, x, y, z) ((z) * (ctx)->dim_size * (ctx)->dim_size + (y) * (ctx)->dim_size + (x))

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
    f32 scale = ctx->grid_size / (f32)(dim - 1);
    f32 offset = ctx->grid_size * 0.5f;

    for (z = 0; z < dim - 1; z++)
    {
        for (y = 0; y < dim - 1; y++)
        {
            for (x = 0; x < dim - 1; x++)
            {
                i32 mask = 0;
                f32 d[8];
                zai_vec3 avg_pos = zai_vec3_init(0.0f, 0.0f, 0.0f);
                i32 intersections = 0;

                for (i = 0; i < 8; i++)
                {
                    i32 cx = x + ((i >> 0) & 1);
                    i32 cy = y + ((i >> 1) & 1);
                    i32 cz = z + ((i >> 2) & 1);

                    d[i] = ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, cx, cy, cz)];

                    if (d[i] < ctx->iso_level)
                    {
                        mask |= (1 << i);
                    }
                }

                if (mask != 0 && mask != 255)
                {
                    static const i32 edge_map[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

                    for (i = 0; i < 12; i++)
                    {
                        i32 i1 = edge_map[i][0];
                        i32 i2 = edge_map[i][1];

                        if (((mask >> i1) & 1) != ((mask >> i2) & 1))
                        {
                            f32 t = (ctx->iso_level - d[i1]) / (d[i2] - d[i1]);
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
                        local_p.x = avg_pos.x * inv_int;
                        local_p.y = avg_pos.y * inv_int;
                        local_p.z = avg_pos.z * inv_int;

                        out_vertices[v_count].position.x = local_p.x * scale - offset + ctx->chunk_coord.x;
                        out_vertices[v_count].position.y = local_p.y * scale - offset + ctx->chunk_coord.y;
                        out_vertices[v_count].position.z = local_p.z * scale - offset + ctx->chunk_coord.z;

                        /* Calculate Normal via Central Difference on the density field */
                        {
                            f32 eps = 1.0f;
                            zai_vec3 n;
                            f32 mag_sq;

                            n.x = ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, (i32)zai_clampf(local_p.x - eps, 0, (f32)dim - 1), (i32)local_p.y, (i32)local_p.z)] -
                                  ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, (i32)zai_clampf(local_p.x + eps, 0, (f32)dim - 1), (i32)local_p.y, (i32)local_p.z)];

                            n.y = ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, (i32)local_p.x, (i32)zai_clampf(local_p.y - eps, 0, (f32)dim - 1), (i32)local_p.z)] -
                                  ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, (i32)local_p.x, (i32)zai_clampf(local_p.y + eps, 0, (f32)dim - 1), (i32)local_p.z)];

                            n.z = ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, (i32)local_p.x, (i32)local_p.y, (i32)zai_clampf(local_p.z - eps, 0, (f32)dim - 1))] -
                                  ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, (i32)local_p.x, (i32)local_p.y, (i32)zai_clampf(local_p.z + eps, 0, (f32)dim - 1))];

                            mag_sq = n.x * n.x + n.y * n.y + n.z * n.z;

                            if (mag_sq > 0.000001f)
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
                        }

                        ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y, z)] = v_count;
                        v_count++;
                    }
                }
                else
                {
                    ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y, z)] = -1;
                }
            }
        }
    }

    for (z = 1; z < dim - 1; z++)
    {
        for (y = 1; y < dim - 1; y++)
        {
            for (x = 1; x < dim - 1; x++)
            {
                f32 d = ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, x, y, z)];

                /* X axis edge */
                if (x < dim - 1)
                {
                    f32 dx = ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, x + 1, y, z)];
                    if ((d < ctx->iso_level) != (dx < ctx->iso_level))
                    {
                        i32 v0 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y, z)];
                        i32 v1 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y - 1, z)];
                        i32 v2 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y - 1, z - 1)];
                        i32 v3 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y, z - 1)];

                        if (v0 != -1 && v1 != -1 && v2 != -1 && v3 != -1)
                        {
                            if (d < ctx->iso_level) /* Inside to Outside */
                            {
                                out_indices[idx_count++] = (u32)v0;
                                out_indices[idx_count++] = (u32)v2;
                                out_indices[idx_count++] = (u32)v1;
                                out_indices[idx_count++] = (u32)v0;
                                out_indices[idx_count++] = (u32)v3;
                                out_indices[idx_count++] = (u32)v2;
                            }
                            else /* Outside to Inside */
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

                /* Y axis edge */
                if (y < dim - 1)
                {
                    f32 dy = ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, x, y + 1, z)];
                    if ((d < ctx->iso_level) != (dy < ctx->iso_level))
                    {
                        i32 v0 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y, z)];
                        i32 v1 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x - 1, y, z)];
                        i32 v2 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x - 1, y, z - 1)];
                        i32 v3 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y, z - 1)];

                        if (v0 != -1 && v1 != -1 && v2 != -1 && v3 != -1)
                        {
                            if (d < ctx->iso_level)
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
                }

                /* Z axis edge */
                if (z < dim - 1)
                {
                    f32 dz = ctx->density_grid[ZAI_SURFACE_NETS_INDEX(ctx, x, y, z + 1)];
                    if ((d < ctx->iso_level) != (dz < ctx->iso_level))
                    {
                        i32 v0 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y, z)];
                        i32 v1 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x - 1, y, z)];
                        i32 v2 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x - 1, y - 1, z)];
                        i32 v3 = ctx->buffer_indices[ZAI_SURFACE_NETS_INDEX(ctx, x, y - 1, z)];

                        if (v0 != -1 && v1 != -1 && v2 != -1 && v3 != -1)
                        {
                            if (d < ctx->iso_level)
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