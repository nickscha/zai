#ifndef ZAI_GEOMETRY_H
#define ZAI_GEOMETRY_H

#include "zai_types.h"

/* #############################################################################
 * # [SECTION] Geometry Generation Functions
 * #############################################################################
 */
ZAI_API ZAI_INLINE i32 zai_geometry_grid(i32 grid_resolution, u16 *grid_indices)
{
    i32 x;
    i32 z;
    i32 i = 0;

    /* indices */
    for (z = 0; z < grid_resolution - 1; ++z)
    {
        for (x = 0; x < grid_resolution - 1; ++x)
        {
            u16 i0 = (u16)(z * grid_resolution + x);
            u16 i1 = i0 + 1;
            u16 i2 = i0 + (u16)grid_resolution;
            u16 i3 = i2 + 1;

            grid_indices[i++] = i0;
            grid_indices[i++] = i2;
            grid_indices[i++] = i1;

            grid_indices[i++] = i1;
            grid_indices[i++] = i2;
            grid_indices[i++] = i3;
        }
    }

    return i;
}

#endif /* ZAI_GEOMETRY_H */