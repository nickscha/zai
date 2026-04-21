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

#endif /* ZAI_MATH_LINEAR_ALGEBRA_H */