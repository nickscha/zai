#version 330 core

in vec2 v_uv;
layout(location = 0) out vec3 outNormal;

uniform vec3 u_tile_origin;

const float TILE_SIZE = 256.0;
const float TEXTURE_SIZE = 1024.0;

vec2 add = vec2(1.0, 0.0);
#define HASHSCALE1 0.1031
const mat2 rotate2D = mat2(1.3623, 1.7531, -1.7131, 1.4623);

float Hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float Noise(vec2 x)
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(Hash12(p),          Hash12(p + add.xy), f.x),
        mix(Hash12(p + add.yx), Hash12(p + add.xx), f.x),
        f.y
    );
}

float Terrain(vec2 p)
{
    vec2 pos = p * 0.05;
    float w = Noise(pos * .25) * .75 + .15;
    float f = 0.0;

    w = 66.0 * w * w;

    for(int i = 0; i < 7; i++)
    {
        f += w * Noise(pos);
        w *= -0.4;
        pos = rotate2D * pos;
    }

    float ff = Noise(pos * .002);
    f += pow(abs(ff), 5.0) * 275.0 - 5.0;

    return f;
}

void main() {
    vec2 worldPos = u_tile_origin.xy * TILE_SIZE + (v_uv - vec2(0.5)) * TILE_SIZE;
    
    vec2 e = vec2(TILE_SIZE / TEXTURE_SIZE, 0.0); 
    float hx = Terrain(worldPos + e.xy) - Terrain(worldPos - e.xy);
    float hz = Terrain(worldPos + e.yx) - Terrain(worldPos - e.yx);
    
    vec3 normal = normalize(vec3(-hx, 2.0 * e.x, -hz));
    
    outNormal = normal * 0.5 + 0.5;
}