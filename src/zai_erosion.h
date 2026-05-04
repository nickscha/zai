#ifndef ZAI_EROSION_H
#define ZAI_EROSION_H

#include "zai_types.h"
#include "zai_math_linear_algebra.h"

/* #############################################################################
 * # [SECTION] Erosion Filter
 * #############################################################################
 */
typedef struct zai_erosion_context
{
    f32 scale;
    f32 erosionScale;
    i32 erosionOctave;
    f32 strength;
    f32 gullyWeight;
    f32 fadeTarget;
    f32 detail;
    zai_vec4 round;
    zai_vec4 onSet;
    f32 roundingMult;
    zai_vec4 assumedSlope;
    zai_vec4 pNoise;

} zai_erosion_context;

ZAI_API ZAI_INLINE f32 zai_erosion_clamp01(f32 t)
{
    return zai_maxf(0.0f, zai_minf(1.0f, t));
}
ZAI_API ZAI_INLINE f32 zai_erosion_lerpf(f32 a, f32 b, f32 t)
{
    return a + t * (b - a);
}
ZAI_API ZAI_INLINE f32 zai_erosion_absf(f32 v)
{
    return v < 0 ? -v : v;
}
ZAI_API ZAI_INLINE f32 zai_erosion_signf(f32 v)
{
    return (v > 0) ? 1.0f : ((v < 0) ? -1.0f : 0.0f);
}

ZAI_API ZAI_INLINE void zai_erosion_sincos(f32 x, f32 *s, f32 *c)
{
    f32 x2 = x * x;
    *s = x * (1.0f - x2 / 6.0f * (1.0f - x2 / 20.0f));
    *c = 1.0f - x2 / 2.0f * (1.0f - x2 / 12.0f);
}

ZAI_API ZAI_INLINE f32 zai_erosion_expf(f32 x)
{
    return 1.0f + x + (x * x) * 0.5f + (x * x * x) * 0.16666f;
}

ZAI_API ZAI_INLINE f32 zai_erosion_powf(f32 x, f32 y)
{
    f32 res = 1.0f;
    i32 i, loops = (i32)y;

    for (i = 0; i < loops; ++i)
    {
        res *= x;
    }
    return res;
}

ZAI_API ZAI_INLINE f32 zai_erosion_ease_out(f32 t)
{
    f32 v = 1.0f - zai_erosion_clamp01(t);
    return 1.0f - (v * v);
}

zai_erosion_context zai_erosion_erosion_default_settings(void)
{
    zai_erosion_context s = {0};

    s.scale = 0.007f;
    s.erosionScale = 2.0f;
    s.erosionOctave = 4;
    s.strength = 0.13f;
    s.gullyWeight = 0.66f;
    s.fadeTarget = 0.63f;
    s.detail = 3.0f;

    s.round.x = 1.63f;
    s.round.y = 2.18f;
    s.round.z = -0.23f;
    s.round.w = 0.0f;

    s.onSet.x = -0.04f;
    s.onSet.y = 1.38f;
    s.onSet.z = 1.0f;
    s.onSet.w = 1.0f;

    s.roundingMult = 0.63f;

    s.assumedSlope.x = 0.75f;
    s.assumedSlope.y = 0.24f;
    s.assumedSlope.z = 0.0f;
    s.assumedSlope.w = 0.0f;

    s.pNoise.x = 0.75f;
    s.pNoise.y = 0.25f;
    s.pNoise.z = 0.75f;
    s.pNoise.w = 0.0f;

    return s;
}

ZAI_API ZAI_INLINE zai_vec3 zai_erosion_hash33(zai_vec3 p)
{
    zai_vec3 r;

    u32 px = (u32)(p.x + 0.5f);
    u32 py = (u32)(p.y + 0.5f);
    u32 pz = (u32)(p.z + 0.5f);

    f32 n;

    px *= 1597334673u;
    py *= 3812015801u;
    pz *= 2798796415u;
    px ^= py ^ pz;
    py ^= px ^ pz;
    pz ^= px ^ py;
    px *= 1597334673u;
    py *= 3812015801u;
    pz *= 2798796415u;

    n = 2.0f / 4294967295.0f;

    r.x = (f32)px * n - 1.0f;
    r.y = (f32)py * n - 1.0f;
    r.z = (f32)pz * n - 1.0f;

    return r;
}

ZAI_API ZAI_INLINE zai_vec2 zai_erosion_phacelle_3d(zai_vec3 p, zai_vec3 dir, zai_vec3 normal, f32 f, f32 offset, f32 norm, zai_vec3 *sideDir)
{
    zai_vec3 sDir = zai_vec3_cross(dir, normal);
    f32 offsetRads = offset * ZAI_TAU;
    zai_vec3 pInt, pFract, phaseDir = zai_vec3_zero;
    f32 weightSum = 0.0f;
    i32 x, y, z;

    zai_vec2 intrep;
    f32 mag;

    sDir.x *= f * ZAI_TAU;
    sDir.y *= f * ZAI_TAU;
    sDir.z *= f * ZAI_TAU;

    *sideDir = sDir;

    pInt.x = (f32)(i32)(p.x + (p.x > 0 ? 0.5f : -0.5f));
    pInt.y = (f32)(i32)(p.y + (p.y > 0 ? 0.5f : -0.5f));
    pInt.z = (f32)(i32)(p.z + (p.z > 0 ? 0.5f : -0.5f));

    pFract.x = p.x - pInt.x;
    pFract.y = p.y - pInt.y;
    pFract.z = p.z - pInt.z;

    for (x = -1; x <= 1; ++x)
    {
        for (y = -1; y <= 1; ++y)
        {
            for (z = -1; z <= 1; ++z)
            {
                zai_vec3 gridPoint = zai_vec3_init(pInt.x + (f32)x, pInt.y + (f32)y, pInt.z + (f32)z);
                zai_vec3 rand = zai_erosion_hash33(gridPoint);
                f32 rLen = zai_sqrtf(zai_vec3_dot(rand, rand));
                zai_vec3 fromCell;
                f32 sqrDst, weight;

                rand.x = (rand.x / rLen) * 0.295f;
                rand.y = (rand.y / rLen) * 0.295f;
                rand.z = (rand.z / rLen) * 0.295f;
                fromCell.x = pFract.x - (f32)x - rand.x;
                fromCell.y = pFract.y - (f32)y - rand.y;
                fromCell.z = pFract.z - (f32)z - rand.z;

                sqrDst = zai_vec3_dot(fromCell, fromCell);
                weight = zai_maxf(0.0f, zai_erosion_expf(-sqrDst * 2.0f) - 0.1111f);

                if (weight > 0.0f)
                {
                    f32 s, c, wave = zai_vec3_dot(fromCell, sDir) + offsetRads;
                    weightSum += weight;
                    zai_erosion_sincos(wave, &s, &c);
                    phaseDir.x += c * weight;
                    phaseDir.y += s * weight;
                }
            }
        }
    }
    weightSum = zai_maxf(weightSum, 0.00001f);

    intrep = zai_vec2_init(phaseDir.x / weightSum, phaseDir.y / weightSum);
    mag = zai_sqrtf(zai_vec2_dot(intrep, intrep));
    mag = zai_maxf(1.0f - norm, mag);
    intrep.x /= mag;
    intrep.y /= mag;

    return intrep;
}

ZAI_API ZAI_INLINE zai_vec4 zai_erosion_filter_3d(zai_vec3 pos, zai_vec3 normal, zai_vec3 dir, f32 height, zai_erosion_context config)
{
    f32 freq = config.erosionScale;
    f32 currentStrength = config.strength;
    f32 heightDelta = 0.0f;
    f32 slopeLength = zai_sqrtf(zai_vec3_dot(dir, dir));
    zai_vec3 gullySlope;
    f32 currentFadeTarget = height, currentMask, ridgeMapCombiMask, ridgeMapFadeTarget, magnitude = 0.0f, currentRoundingMult;
    i32 i;

    f32 rnd;
    f32 ss;

    zai_vec4 res;

    pos.x *= config.scale;
    pos.y *= config.scale;
    pos.z *= config.scale;

    gullySlope.x = zai_erosion_lerpf(dir.x, (dir.x / slopeLength) * config.assumedSlope.x, config.assumedSlope.y);
    gullySlope.y = zai_erosion_lerpf(dir.y, (dir.y / slopeLength) * config.assumedSlope.x, config.assumedSlope.y);
    gullySlope.z = zai_erosion_lerpf(dir.z, (dir.z / slopeLength) * config.assumedSlope.x, config.assumedSlope.y);

    {
        rnd = zai_erosion_lerpf(config.round.y, config.round.x, zai_erosion_clamp01(config.fadeTarget + 0.5f)) * config.round.z;
        ss = zai_maxf(0.0f, (slopeLength * config.onSet.x < rnd * config.onSet.x) ? 0.5f * (slopeLength * config.onSet.x * slopeLength * config.onSet.x) / (rnd * config.onSet.x) : (slopeLength * config.onSet.x) - 0.5f * (rnd * config.onSet.x));
        currentMask = zai_erosion_ease_out(ss);
    }

    ridgeMapCombiMask = zai_erosion_ease_out(slopeLength * config.onSet.z);
    ridgeMapFadeTarget = config.fadeTarget;
    currentRoundingMult = config.roundingMult;

    for (i = 0; i < config.erosionOctave; ++i)
    {
        zai_vec3 sideDir, pFreq = zai_vec3_init(pos.x * freq, pos.y * freq, pos.z * freq);
        zai_vec3 gNorm = gullySlope;
        f32 gL = zai_sqrtf(zai_vec3_dot(gNorm, gNorm));
        zai_vec2 pNoise;
        f32 fadedGullyHeight, rndOct, sloping, newMask;

        gNorm.x /= gL;
        gNorm.y /= gL;
        gNorm.z /= gL;
        pNoise = zai_erosion_phacelle_3d(pFreq, gNorm, normal, config.pNoise.x, config.pNoise.y, config.pNoise.z, &sideDir);

        sideDir.x *= -freq;
        sideDir.y *= -freq;
        sideDir.z *= -freq;
        gullySlope.x += zai_erosion_signf(pNoise.y) * sideDir.x * currentStrength * config.gullyWeight;
        gullySlope.y += zai_erosion_signf(pNoise.y) * sideDir.y * currentStrength * config.gullyWeight;
        gullySlope.z += zai_erosion_signf(pNoise.y) * sideDir.z * currentStrength * config.gullyWeight;

        fadedGullyHeight = zai_erosion_lerpf(currentFadeTarget, pNoise.x * config.gullyWeight, currentMask);
        heightDelta += fadedGullyHeight * currentStrength;
        currentFadeTarget = fadedGullyHeight;

        rndOct = zai_erosion_lerpf(config.round.y, config.round.x, zai_erosion_clamp01(pNoise.x + 0.5f)) * currentRoundingMult;
        sloping = zai_erosion_absf(pNoise.y);
        newMask = zai_erosion_ease_out(zai_maxf(0.0f, (sloping * config.onSet.y < rndOct * config.onSet.y) ? 0.5f * (sloping * config.onSet.y * sloping * config.onSet.y) / (rndOct * config.onSet.y) : (sloping * config.onSet.y) - 0.5f * (rndOct * config.onSet.y)));

        currentMask = (1.0f - zai_erosion_powf(1.0f - zai_erosion_clamp01(currentMask), config.detail)) * newMask;
        ridgeMapFadeTarget = zai_erosion_lerpf(ridgeMapFadeTarget, pNoise.x, ridgeMapCombiMask);
        ridgeMapCombiMask *= zai_erosion_ease_out(sloping * config.onSet.w);

        magnitude += currentStrength;
        currentStrength *= 0.5f;
        freq *= 2.0f;
        currentRoundingMult *= config.round.w;
    }

    res.x = height + heightDelta;
    res.y = ridgeMapFadeTarget * (1.0f - ridgeMapCombiMask);
    res.z = magnitude;
    res.w = heightDelta;

    return res;
}

#endif /* ZAI_EROSION_H */
