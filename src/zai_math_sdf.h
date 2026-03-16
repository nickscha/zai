#ifndef ZAI_MATH_SDF_H
#define ZAI_MATH_SDF_H

#include "zai_math_linear_algebra.h"

/* #############################################################################
 * # [SECTION] Signed Distance Functions
 * #############################################################################
 */
ZAI_API ZAI_INLINE f32 zai_sdf_sphere(zai_vec3 position, f32 radius)
{
    return zai_vec3_length(position) - radius;
}

ZAI_API ZAI_INLINE f32 zai_sdf_octahedron(zai_vec3 position, f32 scale)
{
    position = zai_vec3_abs(position);
    return (position.x + position.y + position.z - scale) * 0.57735027f;
}

ZAI_API ZAI_INLINE f32 zai_sdf_capsule_vertical(zai_vec3 position, f32 height, f32 radius)
{
    position.y -= zai_clampf(position.y, 0.0f, height);
    return zai_vec3_length(position) - radius;
}

ZAI_API ZAI_INLINE f32 zai_sdf_box(zai_vec3 position, zai_vec3 base)
{
    zai_vec3 q = zai_vec3_sub(zai_vec3_abs(position), base);

    return zai_vec3_length(zai_vec3_maxf(q, 0.0f)) + zai_minf(zai_maxf(q.x, zai_maxf(q.y, q.z)), 0.0f);
}

ZAI_API ZAI_INLINE f32 zai_sdf_box_rounded(zai_vec3 position, zai_vec3 base, f32 radius)
{
    zai_vec3 q = zai_vec3_addf(zai_vec3_sub(zai_vec3_abs(position), base), radius);
    f32 lq = zai_vec3_length(zai_vec3_maxf(q, 0.0f));
    f32 mq = zai_minf(zai_maxf(q.x, zai_maxf(q.y, q.z)), 0.0f);

    return lq + mq - radius;
}

ZAI_API ZAI_INLINE f32 zai_sdf_ellipsoid(zai_vec3 position, zai_vec3 radius)
{
    f32 k0 = zai_vec3_length(zai_vec3_div(position, radius));
    f32 k1 = zai_vec3_length(zai_vec3_div(position, zai_vec3_mul(radius, radius)));

    return k0 * (k0 - 1.0f) / k1;
}

ZAI_API ZAI_INLINE f32 zai_sdf_box_frame(zai_vec3 position, zai_vec3 base, f32 edge_thickness)
{
    zai_vec3 p = zai_vec3_sub(zai_vec3_abs(position), base);
    zai_vec3 q = zai_vec3_subf(zai_vec3_abs(zai_vec3_addf(p, edge_thickness)), edge_thickness);

    f32 l1 = zai_vec3_length(zai_vec3_maxf(zai_vec3_init(p.x, q.y, q.z), 0.0f));
    f32 l2 = zai_vec3_length(zai_vec3_maxf(zai_vec3_init(q.x, p.y, q.z), 0.0f));
    f32 l3 = zai_vec3_length(zai_vec3_maxf(zai_vec3_init(q.x, q.y, p.z), 0.0f));

    f32 m1 = zai_minf(zai_maxf(p.x, zai_maxf(q.y, q.z)), 0.0f);
    f32 m2 = zai_minf(zai_maxf(q.x, zai_maxf(p.y, q.z)), 0.0f);
    f32 m3 = zai_minf(zai_maxf(q.x, zai_maxf(q.y, p.z)), 0.0f);

    return zai_minf(zai_minf(l1 + m1, l2 + m2), l3 + m3);
}

/* #############################################################################
 * # [SECTION] Signed Distance Operations Exact
 * #############################################################################
 */
ZAI_API ZAI_INLINE f32 zai_sdf_op_union(f32 a, f32 b)
{
    return zai_minf(a, b);
}

ZAI_API ZAI_INLINE f32 zai_sdf_op_subtract(f32 a, f32 b)
{
    return zai_maxf(-a, b);
}

ZAI_API ZAI_INLINE f32 zai_sdf_op_intersect(f32 a, f32 b)
{
    return zai_maxf(a, b);
}

ZAI_API ZAI_INLINE f32 zai_sdf_op_xor(f32 a, f32 b)
{
    return zai_maxf(zai_minf(a, b), -zai_maxf(a, b));
}

/* #############################################################################
 * # [SECTION] Signed Distance Operations Smooth
 * #############################################################################
 */
ZAI_API ZAI_INLINE f32 zai_sdf_op_union_smooth(f32 a, f32 b, f32 k)
{
    f32 h = zai_maxf(k - zai_absf(a - b), 0.0f) / k;
    return zai_minf(a, b) - h * h * h * k * (1.0f / 6.0f);
}

ZAI_API ZAI_INLINE f32 zai_sdf_op_subtract_smooth(f32 a, f32 b, f32 k)
{
    return -zai_sdf_op_union_smooth(a, -b, k);
}

ZAI_API ZAI_INLINE f32 zai_sdf_op_intersect_smooth(f32 a, f32 b, f32 k)
{
    return -zai_sdf_op_union_smooth(-a, -b, k);
}

/* #############################################################################
 * # [SECTION] Signed Distance Operations Rounding
 * #############################################################################
 */
ZAI_API ZAI_INLINE f32 zai_sdf_op_round(f32 distance, f32 radius)
{
    return distance - radius;
}

ZAI_API ZAI_INLINE f32 zai_sdf_op_onion(f32 distance, f32 thickness)
{
    return zai_absf(distance) - thickness;
}

/* #############################################################################
 * # [SECTION] Signed Distance Operations Symmetry
 * #############################################################################
 */
ZAI_API ZAI_INLINE zai_vec3 zai_sdf_op_symmetric_x(zai_vec3 position)
{
    position.x = zai_absf(position.x);
    return position;
}

ZAI_API ZAI_INLINE zai_vec3 zai_sdf_op_symmetric_xz(zai_vec3 position)
{
    position.x = zai_absf(position.x);
    position.z = zai_absf(position.z);
    return position;
}

/* #############################################################################
 * # [SECTION] Signed Distance Operations Repetition
 * #############################################################################
 */
ZAI_API ZAI_INLINE zai_vec3 zai_sdf_op_repeat(zai_vec3 position, zai_vec3 spacing)
{
    zai_vec3 q;
    zai_vec3 ratio;

    /* ratio = p / s */
    ratio.x = position.x / spacing.x;
    ratio.y = position.y / spacing.y;
    ratio.z = position.z / spacing.z;

    /* q = p - s * round(ratio) */
    q.x = position.x - spacing.x * (f32)((i32)(ratio.x + ((ratio.x >= 0.0f) ? 0.5f : -0.5f)));
    q.y = position.y - spacing.y * (f32)((i32)(ratio.y + ((ratio.y >= 0.0f) ? 0.5f : -0.5f)));
    q.z = position.z - spacing.z * (f32)((i32)(ratio.z + ((ratio.z >= 0.0f) ? 0.5f : -0.5f)));

    return q;
}

ZAI_API ZAI_INLINE zai_vec3 zai_sdf_op_repeat_limited(zai_vec3 position, f32 spacing, zai_vec3 limits)
{
    zai_vec3 q;
    f32 rx, ry, rz;

    /* ratio = p / s */
    rx = position.x / spacing;
    ry = position.y / spacing;
    rz = position.z / spacing;

    /* q = p - s * clamp(round(p / s), -l, l) */
    q.x = position.x - spacing * zai_clampf(zai_roundf(rx), -limits.x, limits.x);
    q.y = position.y - spacing * zai_clampf(zai_roundf(ry), -limits.y, limits.y);
    q.z = position.z - spacing * zai_clampf(zai_roundf(rz), -limits.z, limits.z);

    return q;
}

/* #############################################################################
 * # [SECTION] Signed Distance Axis Aligned Bounding Boxes (AABB)
 * #############################################################################
 */
typedef struct zai_sdf_aabb
{
    zai_vec3 min;
    zai_vec3 max;

} zai_sdf_aabb;

ZAI_API ZAI_INLINE f32 zai_sdf_aabb_distance(zai_vec3 position, zai_sdf_aabb *box)
{
    f32 dx = zai_maxf(0.0f, zai_maxf(box->min.x - position.x, position.x - box->max.x));
    f32 dy = zai_maxf(0.0f, zai_maxf(box->min.y - position.y, position.y - box->max.y));
    f32 dz = zai_maxf(0.0f, zai_maxf(box->min.z - position.z, position.z - box->max.z));

    return (dx * dx) + (dy * dy) + (dz * dz);
}

ZAI_API ZAI_INLINE zai_sdf_aabb zai_sdf_aabb_sphere(zai_vec3 center, f32 radius)
{
    zai_sdf_aabb b;

    b.min.x = center.x - radius;
    b.min.y = center.y - radius;
    b.min.z = center.z - radius;

    b.max.x = center.x + radius;
    b.max.y = center.y + radius;
    b.max.z = center.z + radius;

    return b;
}

#endif /* ZAI_MATH_SDF_H */