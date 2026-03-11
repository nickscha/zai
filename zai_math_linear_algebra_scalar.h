#ifndef ZAI_MATH_LINEAR_ALGEBRA_SCALAR_H
#define ZAI_MATH_LINEAR_ALGEBRA_SCALAR_H

#include "zai_math_basic.h"

/* #############################################################################
 * # [SECTION] Linear Algebra Math (Scalar)
 * #############################################################################
 */
typedef struct zai_vec3
{
    f32 x;
    f32 y;
    f32 z;

} zai_vec3;

static zai_vec3 zai_vec3_zero = {0.0f, 0.0f, 0.0f};

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_init(f32 x, f32 y, f32 z)
{
    zai_vec3 result;

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_initf(f32 value)
{
    zai_vec3 result;

    result.x = value;
    result.y = value;
    result.z = value;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_add(zai_vec3 a, zai_vec3 b)
{
    zai_vec3 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_sub(zai_vec3 a, zai_vec3 b)
{
    zai_vec3 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_mul(zai_vec3 a, zai_vec3 b)
{
    zai_vec3 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_div(zai_vec3 a, zai_vec3 b)
{
    zai_vec3 result;

    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_addf(zai_vec3 a, f32 value)
{
    zai_vec3 result;

    result.x = a.x + value;
    result.y = a.y + value;
    result.z = a.z + value;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_subf(zai_vec3 a, f32 value)
{
    zai_vec3 result;

    result.x = a.x - value;
    result.y = a.y - value;
    result.z = a.z - value;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_mulf(zai_vec3 a, f32 value)
{
    zai_vec3 result;

    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_divf(zai_vec3 a, f32 value)
{
    zai_vec3 result;

    result.x = a.x / value;
    result.y = a.y / value;
    result.z = a.z / value;

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_abs(zai_vec3 a)
{
    zai_vec3 result;

    result.x = zai_absf(a.x);
    result.y = zai_absf(a.y);
    result.z = zai_absf(a.z);

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_minf(zai_vec3 a, f32 value)
{
    zai_vec3 result;

    result.x = zai_minf(a.x, value);
    result.y = zai_minf(a.y, value);
    result.z = zai_minf(a.z, value);

    return result;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_maxf(zai_vec3 a, f32 value)
{
    zai_vec3 result;

    result.x = zai_maxf(a.x, value);
    result.y = zai_maxf(a.y, value);
    result.z = zai_maxf(a.z, value);

    return result;
}

ZAI_API ZAI_INLINE f32 zai_vec3_length(zai_vec3 a)
{
    return zai_sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

ZAI_API ZAI_INLINE f32 zai_vec3_dot(zai_vec3 a, zai_vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

ZAI_API ZAI_INLINE zai_vec3 zai_vec3_cross(zai_vec3 a, zai_vec3 b)
{
    zai_vec3 result;

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return result;
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

#define ZAI_MAT2X2_ELEMENT_COUNT 4

#ifdef ZAI_MAT_ROW_MAJOR_ORDER
#define ZAI_MAT2X2_AT(row, col) ((row) * 2 + (col)) /* Row-major order */
#else
#define ZAI_MAT2X2_AT(row, col) ((col) * 2 + (row)) /* Column-major order */
#endif

typedef struct zai_mat2x2
{
    f32 e[ZAI_MAT2X2_ELEMENT_COUNT];

} zai_mat2x2;

ZAI_API ZAI_INLINE zai_mat2x2 zai_mat2x2_init(f32 m00, f32 m01, f32 m10, f32 m11)
{
    zai_mat2x2 result;

    result.e[ZAI_MAT2X2_AT(0, 0)] = m00;
    result.e[ZAI_MAT2X2_AT(0, 1)] = m01;
    result.e[ZAI_MAT2X2_AT(1, 0)] = m10;
    result.e[ZAI_MAT2X2_AT(1, 1)] = m11;

    return result;
}

ZAI_API ZAI_INLINE zai_mat2x2 zai_mat2x2_rot2d(f32 angle)
{
    f32 s = zai_sinf(angle);
    f32 c = zai_cosf(angle);

    return zai_mat2x2_init(c, -s, s, c);
}

ZAI_API ZAI_INLINE void zai_vec2_mul_mat2x2(f32 *a, f32 *b, zai_mat2x2 m)
{
    f32 x = *a;
    f32 y = *b;

    *a = x * m.e[ZAI_MAT2X2_AT(0, 0)] + y * m.e[ZAI_MAT2X2_AT(1, 0)];
    *b = x * m.e[ZAI_MAT2X2_AT(0, 1)] + y * m.e[ZAI_MAT2X2_AT(1, 1)];
}

#define ZAI_MAT4X4_ELEMENT_COUNT 16

#ifdef ZAI_MAT_ROW_MAJOR_ORDER
#define ZAI_MAT4X4_AT(row, col) ((row) * 4 + (col)) /* Row-major order */
#else
#define ZAI_MAT4X4_AT(row, col) ((col) * 4 + (row)) /* Column-major order */
#endif

typedef struct zai_mat4x4
{
    f32 e[ZAI_MAT4X4_ELEMENT_COUNT];
} zai_mat4x4;

static zai_mat4x4 zai_mat4x4_zero =
    {{0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f}};

ZAI_API ZAI_INLINE zai_mat4x4 zai_mat4x4_mul(zai_mat4x4 a, zai_mat4x4 b)
{
    zai_mat4x4 result;

    i32 i;
    i32 j;

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            result.e[ZAI_MAT4X4_AT(i, j)] =
                a.e[ZAI_MAT4X4_AT(i, 0)] * b.e[ZAI_MAT4X4_AT(0, j)] +
                a.e[ZAI_MAT4X4_AT(i, 1)] * b.e[ZAI_MAT4X4_AT(1, j)] +
                a.e[ZAI_MAT4X4_AT(i, 2)] * b.e[ZAI_MAT4X4_AT(2, j)] +
                a.e[ZAI_MAT4X4_AT(i, 3)] * b.e[ZAI_MAT4X4_AT(3, j)];
        }
    }

    return result;
}

ZAI_API ZAI_INLINE zai_mat4x4 zai_mat4x4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    f32 width = right - left;
    f32 height = top - bottom;
    f32 depth = far - near;

    zai_mat4x4 result = zai_mat4x4_zero;

    result.e[ZAI_MAT4X4_AT(0, 0)] = 2.0f / width;
    result.e[ZAI_MAT4X4_AT(0, 3)] = -(right + left) / width;
    result.e[ZAI_MAT4X4_AT(1, 1)] = 2.0f / height;
    result.e[ZAI_MAT4X4_AT(1, 3)] = -(top + bottom) / height;
    result.e[ZAI_MAT4X4_AT(2, 2)] = -2.0f / depth;
    result.e[ZAI_MAT4X4_AT(2, 3)] = -(far + near) / depth;
    result.e[ZAI_MAT4X4_AT(3, 3)] = 1.0f;

    return result;
}

#endif /* ZAI_MATH_LINEAR_ALGEBRA_SCALAR_H */