#version 330 core

in vec2 v_uv;
layout(location = 0) out vec3 outNormal;

uniform vec3 u_tile_origin; 
uniform float u_tile_size; 
uniform float u_texture_size; 
uniform sampler2D u_stamped_heightmap;  
uniform float u_stamped_heightmap_size; 

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

/* 
 * Catmull-Rom Spline weights for smooth C1 derivatives.
 * Eliminates all grain and faceting from the 65x65 texture.
 */
float w0(float a) { return a * (a * (-0.5 * a + 1.0) - 0.5); }
float w1(float a) { return a * (a * (1.5 * a - 2.5)) + 1.0; }
float w2(float a) { return a * (a * (-1.5 * a + 2.0) + 0.5); }
float w3(float a) { return a * (a * (0.5 * a - 0.5)); }

float SampleHeightmapBicubic(vec2 uv)
{
    vec2 res = vec2(u_stamped_heightmap_size);
    vec2 st = uv * res - 0.5;
    vec2 i = floor(st);
    vec2 f = fract(st);

    float weightsX[4] = float[4](w0(f.x), w1(f.x), w2(f.x), w3(f.x));
    float weightsY[4] = float[4](w0(f.y), w1(f.y), w2(f.y), w3(f.y));

    float h = 0.0;
    for(int y = -1; y <= 2; y++) 
    {
        for(int x = -1; x <= 2; x++) 
        {
            vec2 tc = (i + vec2(float(x), float(y)) + 0.5) / res;
            h += texture(u_stamped_heightmap, tc).r * weightsX[x+1] * weightsY[y+1];
        }
    }
    return h;
}

float Terrain(vec2 uv, vec2 worldPos)
{
    // 1. Get the perfectly smooth base height (including terrain stamps)
    float baseHeight = SampleHeightmapBicubic(uv);

    // 2. Setup noise variables exactly as before
    vec2 pos = worldPos * 0.05;
    float w = Noise(pos * .25) * .75 + .15;
    w = 66.0 * w * w;

    // 3. Fast-forward ONLY 3 octaves (0, 1, 2). 
    // The 65x65 heightmap captures these large shapes perfectly.
    for(int i = 0; i < 3; i++)
    {
        w *= -0.4;
        pos = rotate2D * pos;
    }

    // 4. Generate the remaining 8 octaves (3 through 10).
    // This restores the missing mid-frequency and high-frequency details 
    // that were aliased/blurred out in the 65x65 texture!
    float detailHeight = 0.0;
    for (int i = 0; i < 8; i++)
    {
        detailHeight += w * Noise(pos);
        w =  - w * 0.4;
        pos = rotate2D * pos;
    }

    return baseHeight + detailHeight;
}

void main() {
    vec2 worldPos = u_tile_origin.xy * u_tile_size + (v_uv - vec2(0.5)) * u_tile_size;
    
    vec2 eWorld = vec2(u_tile_size / u_texture_size, 0.0); 
    vec2 eUV = vec2(1.0 / u_texture_size, 0.0);

    float hx = Terrain(v_uv + eUV.xy, worldPos + eWorld.xy) - Terrain(v_uv - eUV.xy, worldPos - eWorld.xy);
    float hz = Terrain(v_uv + eUV.yx, worldPos + eWorld.yx) - Terrain(v_uv - eUV.yx, worldPos - eWorld.yx);
    
    vec3 normal = normalize(vec3(-hx, 2.0 * eWorld.x, -hz));

    outNormal = normal * 0.5 + 0.5;
}