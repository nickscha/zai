#ifndef ZAI_MATH_LINEAR_ALGEBRA_H
#define ZAI_MATH_LINEAR_ALGEBRA_H

#include "zai_math_basic.h"

/* #############################################################################
 * # [SECTION] Linear Algebra Math (SIMD Detection)
 * #############################################################################
 */
#ifdef ZAI_DISABLE_SIMD
#include "zai_math_linear_algebra_scalar.h"
#elif defined(ZAI_ARCH_X64)
#include "zai_math_linear_algebra_sse2.h"
#endif

typedef struct zai_vec2
{
    f32 x;
    f32 y;

} zai_vec2;

ZAI_API ZAI_INLINE zai_vec2 zai_vec2_init(f32 x, f32 y)
{
    zai_vec2 result;

    result.x = x;
    result.y = y;

    return result;
}

ZAI_API ZAI_INLINE f32 zai_vec2_dot(zai_vec2 a, zai_vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

typedef struct zai_vec4
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;

} zai_vec4;

ZAI_API ZAI_INLINE zai_mat4x4 zai_mat4x4_perspective(f32 fov, f32 aspectRatio, f32 zNear, f32 zFar)
{
    f32 f = 1.0f / zai_tanf(fov * 0.5f);
    f32 fn = 1.0f / (zNear - zFar);

    zai_mat4x4 result = zai_mat4x4_zero;

    result.e[ZAI_MAT4X4_AT(0, 0)] = f / aspectRatio;
    result.e[ZAI_MAT4X4_AT(1, 1)] = f;
    result.e[ZAI_MAT4X4_AT(2, 2)] = (zNear + zFar) * fn;
    result.e[ZAI_MAT4X4_AT(2, 3)] = (2.0f * zNear * zFar) * fn;
    result.e[ZAI_MAT4X4_AT(3, 2)] = -1.0f;

    return (result);
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_normalize(zai_vec3 a)
{
    f32 length_squared = a.x * a.x + a.y * a.y + a.z * a.z;
    f32 scalar = length_squared > 0.0f ? zai_invsqrtf(length_squared) : 0.0f;

    zai_vec3 result;

    result.x = a.x * scalar;
    result.y = a.y * scalar;
    result.z = a.z * scalar;

    return result;
}

ZAI_API ZAI_INLINE zai_mat4x4 zai_mat4x4_look_at(zai_vec3 eye, zai_vec3 target, zai_vec3 up)
{
    zai_vec3 f = zai_vec3_normalize(zai_vec3_sub(target, eye));
    zai_vec3 s = zai_vec3_normalize(zai_vec3_cross(f, up));
    zai_vec3 u = zai_vec3_normalize(zai_vec3_cross(s, f));

    zai_mat4x4 result;

    result.e[ZAI_MAT4X4_AT(0, 0)] = s.x;
    result.e[ZAI_MAT4X4_AT(1, 0)] = u.x;
    result.e[ZAI_MAT4X4_AT(2, 0)] = -f.x;
    result.e[ZAI_MAT4X4_AT(3, 0)] = 0.0f;

    result.e[ZAI_MAT4X4_AT(0, 1)] = s.y;
    result.e[ZAI_MAT4X4_AT(1, 1)] = u.y;
    result.e[ZAI_MAT4X4_AT(2, 1)] = -f.y;
    result.e[ZAI_MAT4X4_AT(3, 1)] = 0.0f;

    result.e[ZAI_MAT4X4_AT(0, 2)] = s.z;
    result.e[ZAI_MAT4X4_AT(1, 2)] = u.z;
    result.e[ZAI_MAT4X4_AT(2, 2)] = -f.z;
    result.e[ZAI_MAT4X4_AT(3, 2)] = 0.0f;

    result.e[ZAI_MAT4X4_AT(0, 3)] = -zai_vec3_dot(s, eye);
    result.e[ZAI_MAT4X4_AT(1, 3)] = -zai_vec3_dot(u, eye);
    result.e[ZAI_MAT4X4_AT(2, 3)] = zai_vec3_dot(f, eye);
    result.e[ZAI_MAT4X4_AT(3, 3)] = 1.0f;

    return result;
}

typedef struct zai_frustum
{
    zai_vec4 planes[6]; /* [0]=near, [1]=far, [2]=left, [3]=right, [4]=top, [5]=bottom */
} zai_frustum;

ZAI_API ZAI_INLINE void zai_frustum_extract_planes(f32 *mvp, zai_frustum *frustum)
{
    u32 i;

    /* Right plane */
    frustum->planes[2].x = mvp[3] - mvp[0];
    frustum->planes[2].y = mvp[7] - mvp[4];
    frustum->planes[2].z = mvp[11] - mvp[8];
    frustum->planes[2].w = mvp[15] - mvp[12];

    /* Left plane */
    frustum->planes[3].x = mvp[3] + mvp[0];
    frustum->planes[3].y = mvp[7] + mvp[4];
    frustum->planes[3].z = mvp[11] + mvp[8];
    frustum->planes[3].w = mvp[15] + mvp[12];

    /* Top plane */
    frustum->planes[4].x = mvp[3] - mvp[1];
    frustum->planes[4].y = mvp[7] - mvp[5];
    frustum->planes[4].z = mvp[11] - mvp[9];
    frustum->planes[4].w = mvp[15] - mvp[13];

    /* Bottom plane */
    frustum->planes[5].x = mvp[3] + mvp[1];
    frustum->planes[5].y = mvp[7] + mvp[5];
    frustum->planes[5].z = mvp[11] + mvp[9];
    frustum->planes[5].w = mvp[15] + mvp[13];

    /* Near plane */
    frustum->planes[0].x = mvp[3] + mvp[2];
    frustum->planes[0].y = mvp[7] + mvp[6];
    frustum->planes[0].z = mvp[11] + mvp[10];
    frustum->planes[0].w = mvp[15] + mvp[14];

    /* Far plane */
    frustum->planes[1].x = mvp[3] - mvp[2];
    frustum->planes[1].y = mvp[7] - mvp[6];
    frustum->planes[1].z = mvp[11] - mvp[10];
    frustum->planes[1].w = mvp[15] - mvp[14];

    for (i = 0; i < 6; ++i)
    {
        f32 len = zai_sqrtf(frustum->planes[i].x * frustum->planes[i].x +
                            frustum->planes[i].y * frustum->planes[i].y +
                            frustum->planes[i].z * frustum->planes[i].z);

        frustum->planes[i].x /= len;
        frustum->planes[i].y /= len;
        frustum->planes[i].z /= len;
        frustum->planes[i].w /= len;
    }
}

ZAI_API ZAI_INLINE u8 zai_frustum_is_sphere_visible(zai_frustum *frustum, f32 cx, f32 cy, f32 cz, f32 radius)
{
    u32 i;

    for (i = 0; i < 6; ++i)
    {
        f32 dist = frustum->planes[i].x * cx +
                   frustum->planes[i].y * cy +
                   frustum->planes[i].z * cz +
                   frustum->planes[i].w;

        if (dist < -radius)
        {
            return 0; /* Outside */
        }
    }

    return 1;
}

ZAI_API ZAI_INLINE u8 zai_frustum_is_aabb_visible(zai_frustum *frustum, f32 minx, f32 miny, f32 minz, f32 maxx, f32 maxy, f32 maxz)
{
    u32 i;

    for (i = 0; i < 6; ++i)
    {
        f32 p = frustum->planes[i].x * (frustum->planes[i].x > 0 ? maxx : minx) +
                frustum->planes[i].y * (frustum->planes[i].y > 0 ? maxy : miny) +
                frustum->planes[i].z * (frustum->planes[i].z > 0 ? maxz : minz) +
                frustum->planes[i].w;

        if (p < 0)
        {
            return 0;
        }
    }

    return 1;
}

#endif /* ZAI_MATH_LINEAR_ALGEBRA_H */