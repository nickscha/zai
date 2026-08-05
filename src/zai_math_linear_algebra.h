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
    zai_vec4 planes[6];
} zai_frustum;

typedef enum zai_frustum_plane
{
    ZAI_FRUSTUM_LEFT = 0,
    ZAI_FRUSTUM_RIGHT,
    ZAI_FRUSTUM_BOTTOM,
    ZAI_FRUSTUM_TOP,
    ZAI_FRUSTUM_NEAR,
    ZAI_FRUSTUM_FAR,
    ZAI_FRUSTUM_COUNT

} zai_frustum_plane;

ZAI_API zai_frustum zai_frustum_extract(zai_mat4x4 m)
{
    zai_frustum f = {0};

    u32 i;

    f.planes[ZAI_FRUSTUM_LEFT].x = m.e[3] + m.e[0];
    f.planes[ZAI_FRUSTUM_LEFT].y = m.e[7] + m.e[4];
    f.planes[ZAI_FRUSTUM_LEFT].z = m.e[11] + m.e[8];
    f.planes[ZAI_FRUSTUM_LEFT].w = m.e[15] + m.e[12];
    f.planes[ZAI_FRUSTUM_RIGHT].x = m.e[3] - m.e[0];
    f.planes[ZAI_FRUSTUM_RIGHT].y = m.e[7] - m.e[4];
    f.planes[ZAI_FRUSTUM_RIGHT].z = m.e[11] - m.e[8];
    f.planes[ZAI_FRUSTUM_RIGHT].w = m.e[15] - m.e[12];
    f.planes[ZAI_FRUSTUM_BOTTOM].x = m.e[3] + m.e[1];
    f.planes[ZAI_FRUSTUM_BOTTOM].y = m.e[7] + m.e[5];
    f.planes[ZAI_FRUSTUM_BOTTOM].z = m.e[11] + m.e[9];
    f.planes[ZAI_FRUSTUM_BOTTOM].w = m.e[15] + m.e[13];
    f.planes[ZAI_FRUSTUM_TOP].x = m.e[3] - m.e[1];
    f.planes[ZAI_FRUSTUM_TOP].y = m.e[7] - m.e[5];
    f.planes[ZAI_FRUSTUM_TOP].z = m.e[11] - m.e[9];
    f.planes[ZAI_FRUSTUM_TOP].w = m.e[15] - m.e[13];
    f.planes[ZAI_FRUSTUM_NEAR].x = m.e[3] + m.e[2];
    f.planes[ZAI_FRUSTUM_NEAR].y = m.e[7] + m.e[6];
    f.planes[ZAI_FRUSTUM_NEAR].z = m.e[11] + m.e[10];
    f.planes[ZAI_FRUSTUM_NEAR].w = m.e[15] + m.e[14];
    f.planes[ZAI_FRUSTUM_FAR].x = m.e[3] - m.e[2];
    f.planes[ZAI_FRUSTUM_FAR].y = m.e[7] - m.e[6];
    f.planes[ZAI_FRUSTUM_FAR].z = m.e[11] - m.e[10];
    f.planes[ZAI_FRUSTUM_FAR].w = m.e[15] - m.e[14];

    for (i = 0; i < 6; ++i)
    {
        zai_vec4 *p = &f.planes[i];

        f32 len = zai_sqrtf(
            p->x * p->x +
            p->y * p->y +
            p->z * p->z);

        if (len > 0.0f)
        {
            p->x /= len;
            p->y /= len;
            p->z /= len;
            p->w /= len;
        }
    }

    return f;
}

#endif /* ZAI_MATH_LINEAR_ALGEBRA_H */