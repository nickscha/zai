#ifndef ZAI_ZAI_H
#define ZAI_ZAI_H

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
        zai_noise_swap_byte(&zai_noise_permutations[i], &zai_noise_permutations[(int)r]);
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

ZAI_API ZAI_INLINE f32 zai_noise_lerp(f32 a, f32 b, f32 t)
{
    return a + t * (b - a);
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

    x1 = zai_noise_lerp(zai_noise_dot2(zai_noise_gradient_2_lut[aa & 7], xf, yf),
                        zai_noise_dot2(zai_noise_gradient_2_lut[ba & 7], xf - 1, yf), u);
    x2 = zai_noise_lerp(zai_noise_dot2(zai_noise_gradient_2_lut[ab & 7], xf, yf - 1),
                        zai_noise_dot2(zai_noise_gradient_2_lut[bb & 7], xf - 1, yf - 1), u);
    y1 = zai_noise_lerp(x1, x2, v);

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

    x1 = zai_noise_lerp(zai_noise_dot3(zai_noise_gradient_3_lut[aaa & 15], xf, yf, zf),
                        zai_noise_dot3(zai_noise_gradient_3_lut[baa & 15], xf - 1, yf, zf), u);
    x2 = zai_noise_lerp(zai_noise_dot3(zai_noise_gradient_3_lut[aba & 15], xf, yf - 1, zf),
                        zai_noise_dot3(zai_noise_gradient_3_lut[bba & 15], xf - 1, yf - 1, zf), u);
    y1 = zai_noise_lerp(x1, x2, v);

    x1 = zai_noise_lerp(zai_noise_dot3(zai_noise_gradient_3_lut[aab & 15], xf, yf, zf - 1),
                        zai_noise_dot3(zai_noise_gradient_3_lut[bab & 15], xf - 1, yf, zf - 1), u);
    x2 = zai_noise_lerp(zai_noise_dot3(zai_noise_gradient_3_lut[abb & 15], xf, yf - 1, zf - 1),
                        zai_noise_dot3(zai_noise_gradient_3_lut[bbb & 15], xf - 1, yf - 1, zf - 1), u);
    y2 = zai_noise_lerp(x1, x2, v);

    return zai_noise_lerp(y1, y2, w) * 0.70710678f; /* normalize -1 to 1 */
}

#endif /* ZAI_ZAI_H */