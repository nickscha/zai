#version 330 core

in vec2 v_uv;
layout(location = 0) out vec3 outNormal;

uniform vec3 u_tile_origin; /* where the tile starts */
uniform float u_tile_size; /* size of a tile (e.g 256.0f world units) */
uniform float u_texture_size; /* size of the output normal map */
uniform sampler2D u_stamped_heightmap;  /* gl_nearest low res 5 octaves noise heightmap with stamp terrain modifications */
uniform float u_stamped_heightmap_size; /* size of the heightmap */

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

    /* 5 octaves which the low res heightmap use */
    for(int i = 0; i < 5; i++)
    {
        f += w * Noise(pos);
        w *= -0.4;
        pos = rotate2D * pos;
    }

    float ff = Noise(pos * .002);
    f += pow(abs(ff), 5.0) * 275.0 - 5.0;

    /* add 6 more octaves of detail for high res normal map */
	for (int i = 0; i < 6; i++)
	{
		f += w * Noise(pos);
		w =  - w * 0.4;
		pos = rotate2D * pos;
	}

    return f;
}

void main() {
    vec2 worldPos = u_tile_origin.xy * u_tile_size + (v_uv - vec2(0.5)) * u_tile_size;
    
    vec2 e = vec2(u_tile_size / u_texture_size, 0.0); 
    float hx = Terrain(worldPos + e.xy) - Terrain(worldPos - e.xy);
    float hz = Terrain(worldPos + e.yx) - Terrain(worldPos - e.yx);
    
    vec3 normal = normalize(vec3(-hx, 2.0 * e.x, -hz));

    outNormal = normal * 0.5 + 0.5;
}