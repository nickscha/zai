#ifndef ZAI_NOISE_H
#define ZAI_NOISE_H

#include "zai_types.h"
#include "zai_math_linear_algebra.h"

/* #############################################################################
 * # [SECTION] Perlin Noise
 * #############################################################################
 */
static u8 zai_noise_permutations[512];
static u32 zai_noise_lcg_state;
static f32 zai_noise_gradient_2_lut[8][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
static f32 zai_noise_gradient_3_lut[16][3] = {{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0}, {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1}, {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}, {1, 1, 0}, {-1, 1, 0}, {0, -1, 1}, {0, -1, -1}};

ZAI_API ZAI_INLINE void zai_noise_swap_byte(u8 *a, u8 *b)
{
    u8 t = *a;
    *a = *b;
    *b = t;
}

ZAI_API ZAI_INLINE unsigned zai_noise_lcg_next(void)
{
    zai_noise_lcg_state = zai_noise_lcg_state * 1664525u + 1013904223u;
    return zai_noise_lcg_state;
}

ZAI_API ZAI_INLINE void zai_noise_seed(u32 seed)
{
    i32 i;
    zai_noise_lcg_state = seed;

    for (i = 0; i < 256; ++i)
    {
        zai_noise_permutations[i] = (u8)i;
    }

    for (i = 255; i > 0; --i)
    {
        unsigned r = zai_noise_lcg_next() % (u32)(i + 1);
        zai_noise_swap_byte(&zai_noise_permutations[i], &zai_noise_permutations[(i32)r]);
    }

    for (i = 0; i < 256; ++i)
    {
        zai_noise_permutations[256 + i] = zai_noise_permutations[i];
    }
}

ZAI_API ZAI_INLINE f32 zai_noise_fade(f32 t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

ZAI_API ZAI_INLINE f32 zai_noise_dot2(f32 g[2], f32 x, f32 y)
{
    return g[0] * x + g[1] * y;
}

ZAI_API ZAI_INLINE f32 zai_noise_dot3(f32 g[3], f32 x, f32 y, f32 z)
{
    return g[0] * x + g[1] * y + g[2] * z;
}

ZAI_API ZAI_INLINE f32 zai_noise_perlin_2(f32 x, f32 y, f32 frequency)
{
    u8 *perm = zai_noise_permutations;
    i32 X, Y, aa, ab, ba, bb;
    f32 xf, yf, u, v, x1, x2, y1;
    f32 floor_x, floor_y;

    x *= frequency;
    y *= frequency;

    floor_x = zai_floorf(x);
    floor_y = zai_floorf(y);

    X = (i32)floor_x & 255;
    Y = (i32)floor_y & 255;
    xf = x - floor_x;
    yf = y - floor_y;
    u = zai_noise_fade(xf);
    v = zai_noise_fade(yf);

    aa = perm[perm[X] + Y];
    ab = perm[perm[X] + Y + 1];
    ba = perm[perm[X + 1] + Y];
    bb = perm[perm[X + 1] + Y + 1];

    x1 = zai_lerpf(zai_noise_dot2(zai_noise_gradient_2_lut[aa & 7], xf, yf),
                   zai_noise_dot2(zai_noise_gradient_2_lut[ba & 7], xf - 1, yf), u);
    x2 = zai_lerpf(zai_noise_dot2(zai_noise_gradient_2_lut[ab & 7], xf, yf - 1),
                   zai_noise_dot2(zai_noise_gradient_2_lut[bb & 7], xf - 1, yf - 1), u);
    y1 = zai_lerpf(x1, x2, v);

    return y1 * 0.70710678f; /* normalize -1 to 1 */
}

ZAI_API ZAI_INLINE f32 zai_noise_perlin_3(f32 x, f32 y, f32 z, f32 freq)
{
    u8 *perm = zai_noise_permutations;
    i32 X, Y, Z, aaa, aba, aab, abb, baa, bba, bab, bbb;
    f32 xf, yf, zf, u, v, w, x1, x2, y1, y2;
    f32 floor_x, floor_y, floor_z;

    x *= freq;
    y *= freq;
    z *= freq;

    floor_x = zai_floorf(x);
    floor_y = zai_floorf(y);
    floor_z = zai_floorf(z);

    X = (i32)floor_x & 255;
    Y = (i32)floor_y & 255;
    Z = (i32)floor_z & 255;
    xf = x - floor_x;
    yf = y - floor_y;
    zf = z - floor_z;
    u = zai_noise_fade(xf);
    v = zai_noise_fade(yf);
    w = zai_noise_fade(zf);

    aaa = perm[perm[perm[X] + Y] + Z];
    aba = perm[perm[perm[X] + Y + 1] + Z];
    aab = perm[perm[perm[X] + Y] + Z + 1];
    abb = perm[perm[perm[X] + Y + 1] + Z + 1];
    baa = perm[perm[perm[X + 1] + Y] + Z];
    bba = perm[perm[perm[X + 1] + Y + 1] + Z];
    bab = perm[perm[perm[X + 1] + Y] + Z + 1];
    bbb = perm[perm[perm[X + 1] + Y + 1] + Z + 1];

    x1 = zai_lerpf(zai_noise_dot3(zai_noise_gradient_3_lut[aaa & 15], xf, yf, zf),
                   zai_noise_dot3(zai_noise_gradient_3_lut[baa & 15], xf - 1, yf, zf), u);
    x2 = zai_lerpf(zai_noise_dot3(zai_noise_gradient_3_lut[aba & 15], xf, yf - 1, zf),
                   zai_noise_dot3(zai_noise_gradient_3_lut[bba & 15], xf - 1, yf - 1, zf), u);
    y1 = zai_lerpf(x1, x2, v);

    x1 = zai_lerpf(zai_noise_dot3(zai_noise_gradient_3_lut[aab & 15], xf, yf, zf - 1),
                   zai_noise_dot3(zai_noise_gradient_3_lut[bab & 15], xf - 1, yf, zf - 1), u);
    x2 = zai_lerpf(zai_noise_dot3(zai_noise_gradient_3_lut[abb & 15], xf, yf - 1, zf - 1),
                   zai_noise_dot3(zai_noise_gradient_3_lut[bbb & 15], xf - 1, yf - 1, zf - 1), u);
    y2 = zai_lerpf(x1, x2, v);

    return zai_lerpf(y1, y2, w) * 0.70710678f; /* normalize -1 to 1 */
}

ZAI_API ZAI_INLINE f32 zai_noise_perlin_3_fbm(f32 x, f32 y, f32 z, f32 frequency, i32 octaves, f32 lacunarity, f32 gain)
{
    i32 i;
    f32 sum = 0, amp = 1, f = frequency, norm = 0;

    for (i = 0; i < octaves; ++i)
    {
        sum += amp * zai_noise_perlin_3(x, y, z, f);
        norm += amp;
        f *= lacunarity;
        amp *= gain;
    }

    return sum / norm;
}

/* #############################################################################
 * # [SECTION] Perlin Noise Fast
 * #############################################################################
 */
#define ZAI_NOISE_FADE(t) (t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f))
#define ZAI_NOISE_LERP(t, a, b) (a + t * (b - a))

ZAI_API ZAI_INLINE i32 zai_noise_floorf(f32 x)
{
    i32 i = (i32)x;
    return (x < (f32)i) ? i - 1 : i;
}

ZAI_API ZAI_INLINE f32 zai_noise_grad_dot(i32 hash, f32 x, f32 y, f32 z)
{
    i32 h = hash & 15;
    f32 u = h < 8 ? x : y;
    f32 v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

ZAI_API ZAI_INLINE i32 zai_noise_hash_coordinates(i32 x, i32 y, i32 z, i32 seed)
{
    i32 h = seed ^ x;
    h = (h ^ y) * 0x27d4eb2d;
    h = (h ^ z) * 0x27d4eb2d;
    h = h ^ (h >> 15);
    return h;
}

ZAI_API ZAI_INLINE void zai_noise_m3x3_mul(f32 m[3][3], f32 v[3], f32 out[3])
{
    out[0] = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
    out[1] = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2];
    out[2] = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2];
}

ZAI_API ZAI_INLINE f32 zai_noise_3d(f32 x, f32 y, f32 z, i32 seed)
{
    i32 X0 = zai_noise_floorf(x);
    i32 Y0 = zai_noise_floorf(y);
    i32 Z0 = zai_noise_floorf(z);

    i32 X1 = X0 + 1;
    i32 Y1 = Y0 + 1;
    i32 Z1 = Z0 + 1;

    f32 xf = x - (f32)X0;
    f32 yf = y - (f32)Y0;
    f32 zf = z - (f32)Z0;

    f32 u = ZAI_NOISE_FADE(xf);
    f32 v = ZAI_NOISE_FADE(yf);
    f32 w = ZAI_NOISE_FADE(zf);

    /* Calculate gradients for all 8 corners of the cell */
    f32 g000 = zai_noise_grad_dot(zai_noise_hash_coordinates(X0, Y0, Z0, seed), xf, yf, zf);
    f32 g100 = zai_noise_grad_dot(zai_noise_hash_coordinates(X1, Y0, Z0, seed), xf - 1.0f, yf, zf);
    f32 g010 = zai_noise_grad_dot(zai_noise_hash_coordinates(X0, Y1, Z0, seed), xf, yf - 1.0f, zf);
    f32 g110 = zai_noise_grad_dot(zai_noise_hash_coordinates(X1, Y1, Z0, seed), xf - 1.0f, yf - 1.0f, zf);
    f32 g001 = zai_noise_grad_dot(zai_noise_hash_coordinates(X0, Y0, Z1, seed), xf, yf, zf - 1.0f);
    f32 g101 = zai_noise_grad_dot(zai_noise_hash_coordinates(X1, Y0, Z1, seed), xf - 1.0f, yf, zf - 1.0f);
    f32 g011 = zai_noise_grad_dot(zai_noise_hash_coordinates(X0, Y1, Z1, seed), xf, yf - 1.0f, zf - 1.0f);
    f32 g111 = zai_noise_grad_dot(zai_noise_hash_coordinates(X1, Y1, Z1, seed), xf - 1.0f, yf - 1.0f, zf - 1.0f);

    /* Trilinear interpolation */
    f32 lerp_x0 = ZAI_NOISE_LERP(u, g000, g100);
    f32 lerp_x1 = ZAI_NOISE_LERP(u, g010, g110);
    f32 lerp_x2 = ZAI_NOISE_LERP(u, g001, g101);
    f32 lerp_x3 = ZAI_NOISE_LERP(u, g011, g111);

    f32 lerp_y0 = ZAI_NOISE_LERP(v, lerp_x0, lerp_x1);
    f32 lerp_y1 = ZAI_NOISE_LERP(v, lerp_x2, lerp_x3);

    return ZAI_NOISE_LERP(w, lerp_y0, lerp_y1);
}

ZAI_API ZAI_INLINE f32 zai_noise_3d_fbm(f32 x, f32 y, f32 z, f32 frequency, i32 octaves, f32 lacunarity, f32 gain, i32 seed)
{
    i32 i;
    f32 sum = 0, amp = 1, f = frequency, norm = 0;

    for (i = 0; i < octaves; ++i)
    {
        sum += amp * zai_noise_3d(x * f, y * f, z * f, seed);
        norm += amp;
        f *= lacunarity;
        amp *= gain;
    }

    return sum / norm;
}

ZAI_API ZAI_INLINE f32 zai_noise_3d_fbm_rotation(f32 x, f32 y, f32 z, f32 frequency, i32 octaves, f32 lacunarity, f32 gain, i32 seed, f32 rotation[3][3])
{
    i32 i;
    f32 sum = 0.0f, amp = 1.0f, norm = 0.0f;
    f32 p[3];

    p[0] = x * frequency;
    p[1] = y * frequency;
    p[2] = z * frequency;

    for (i = 0; i < octaves; ++i)
    {
        f32 tmp[3];

        /* sample noise */
        sum += amp * zai_noise_3d(p[0], p[1], p[2], seed);
        norm += amp;

        /* rotate then scale */
        zai_noise_m3x3_mul(rotation, p, tmp);
        p[0] = tmp[0] * lacunarity;
        p[1] = tmp[1] * lacunarity;
        p[2] = tmp[2] * lacunarity;

        amp *= gain;
    }

    return sum / norm;
}

/* #############################################################################
 * # [SECTION] Value Noise
 * #############################################################################
 */
ZAI_API ZAI_INLINE u32 zai_value_noise_hash3(u32 x, u32 y, u32 z)
{
    u32 h = x * 374761393u + y * 668265263u + z * 2147483647u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

ZAI_API ZAI_INLINE f32 zai_value_noise_hash_to_f32(u32 h)
{
    return ((f32)(h & 0xFFFFFF) / 16777215.0f) * 2.0f - 1.0f;
}

ZAI_API ZAI_INLINE f32 zai_noise_smooth(f32 t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

ZAI_API ZAI_INLINE f32 zai_noise_lerp(f32 a, f32 b, f32 t)
{
    return a + t * (b - a);
}

ZAI_API ZAI_INLINE f32 zai_value_noise_3d(f32 x, f32 y, f32 z)
{
    i32 xi = zai_noise_floorf(x);
    i32 yi = zai_noise_floorf(y);
    i32 zi = zai_noise_floorf(z);

    f32 xf = x - (f32)xi;
    f32 yf = y - (f32)yi;
    f32 zf = z - (f32)zi;

    f32 u = zai_noise_smooth(xf);
    f32 v = zai_noise_smooth(yf);
    f32 w = zai_noise_smooth(zf);

    /* Hash corners */
    u32 X = (u32)xi;
    u32 Y = (u32)yi;
    u32 Z = (u32)zi;

    f32 c000 = zai_value_noise_hash_to_f32(zai_value_noise_hash3(X, Y, Z));
    f32 c100 = zai_value_noise_hash_to_f32(zai_value_noise_hash3(X + 1, Y, Z));
    f32 c010 = zai_value_noise_hash_to_f32(zai_value_noise_hash3(X, Y + 1, Z));
    f32 c110 = zai_value_noise_hash_to_f32(zai_value_noise_hash3(X + 1, Y + 1, Z));
    f32 c001 = zai_value_noise_hash_to_f32(zai_value_noise_hash3(X, Y, Z + 1));
    f32 c101 = zai_value_noise_hash_to_f32(zai_value_noise_hash3(X + 1, Y, Z + 1));
    f32 c011 = zai_value_noise_hash_to_f32(zai_value_noise_hash3(X, Y + 1, Z + 1));
    f32 c111 = zai_value_noise_hash_to_f32(zai_value_noise_hash3(X + 1, Y + 1, Z + 1));

    /* Trilinear interpolation */
    f32 x00 = zai_noise_lerp(c000, c100, u);
    f32 x10 = zai_noise_lerp(c010, c110, u);
    f32 x01 = zai_noise_lerp(c001, c101, u);
    f32 x11 = zai_noise_lerp(c011, c111, u);

    f32 y0 = zai_noise_lerp(x00, x10, v);
    f32 y1 = zai_noise_lerp(x01, x11, v);

    return zai_noise_lerp(y0, y1, w);
}

ZAI_API ZAI_INLINE f32 zai_value_noise_3d_fbm(f32 x, f32 y, f32 z, f32 frequency, i32 octaves, f32 lacunarity, f32 gain)
{
    i32 i;
    f32 sum = 0, amp = 1, f = frequency, norm = 0;

    for (i = 0; i < octaves; ++i)
    {
        sum += amp * zai_value_noise_3d(x * f, y * f, z * f);
        norm += amp;
        f *= lacunarity;
        amp *= gain;
    }

    return sum / norm;
}

ZAI_API ZAI_INLINE f32 zai_value_noise_3d_fbm_rotation(f32 x, f32 y, f32 z, f32 frequency, i32 octaves, f32 lacunarity, f32 gain, f32 rotation[3][3])
{
    i32 i;
    f32 sum = 0.0f, amp = 1.0f, norm = 0.0f;
    f32 p[3];

    p[0] = x * frequency;
    p[1] = y * frequency;
    p[2] = z * frequency;

    for (i = 0; i < octaves; ++i)
    {
        f32 tmp[3];

        /* sample noise */
        sum += amp * zai_value_noise_3d(p[0], p[1], p[2]);
        norm += amp;

        /* rotate then scale */
        zai_noise_m3x3_mul(rotation, p, tmp);
        p[0] = tmp[0] * lacunarity;
        p[1] = tmp[1] * lacunarity;
        p[2] = tmp[2] * lacunarity;

        amp *= gain;
    }

    return sum / norm;
}

#endif /* ZAI_NOISE_H */