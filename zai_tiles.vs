#version 330 core

uniform vec3 u_tile_offset;
uniform mat4 u_vp;
uniform int u_is_dirty;

out vec3 vColor;
out float v_is_dirty_pass;

const float GRID_RES = 65.0;
const float TILE_SIZE = 256.0;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

vec2 add = vec2(1.0, 0.0);
#define HASHSCALE1 .1031
const mat2 rotate2D = mat2(1.3623, 1.7531, -1.7131, 1.4623);

float Hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float Noise( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    
    float res = mix(mix( Hash12(p),          Hash12(p + add.xy),f.x),
                    mix( Hash12(p + add.yx), Hash12(p + add.xx),f.x),f.y);
    return res;
}

float Terrain( in vec2 p)
{
	vec2 pos = p*0.05;
	float w = (Noise(pos*.25)*0.75+.15);
	w = 66.0 * w * w;
	vec2 dxy = vec2(0.0, 0.0);
	float f = .0;
	for (int i = 0; i < 5; i++)
	{
		f += w * Noise(pos);
		w = -w * 0.4;
		pos = rotate2D * pos;
	}
	float ff = Noise(pos*.002);
	
	f += pow(abs(ff), 5.0)*275.-5.0;
	return f;
}

void main()
{
    float v_x = float(gl_VertexID % int(GRID_RES));
    float v_z = float(gl_VertexID / int(GRID_RES));

    vec2 local_pos = vec2(v_x, v_z) / (GRID_RES - 1.0) * TILE_SIZE;    
    vec2 world_pos = local_pos + (u_tile_offset.xy * TILE_SIZE);

    float height = Terrain(world_pos);
    
    gl_Position = u_vp * vec4(world_pos.x, height, world_pos.y, 1.0);

    vec2 tile_grid_coord = floor(u_tile_offset.xy + vec2(0.5));
    
    float r = hash(tile_grid_coord + vec2(0.0, 0.0)) * 0.5;
    float g = hash(tile_grid_coord + vec2(1.0, 4.3));
    float b = hash(tile_grid_coord + vec2(2.5, 8.1));

    vColor = vec3(r, g, b);
    v_is_dirty_pass = float(u_is_dirty);
}