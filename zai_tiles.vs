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

void main()
{
    float v_x = float(gl_VertexID % int(GRID_RES));
    float v_z = float(gl_VertexID / int(GRID_RES));

    vec2 local_pos = vec2(v_x / (GRID_RES - 1.0), -v_z / (GRID_RES - 1.0)) * TILE_SIZE;    
    vec2 world_pos = local_pos + (u_tile_offset.xy * TILE_SIZE);
    
    gl_Position = u_vp * vec4(world_pos.x, 0.0, world_pos.y, 1.0);

    vec2 tile_grid_coord = floor(u_tile_offset.xy + vec2(0.5));
    
    float r = hash(tile_grid_coord + vec2(0.0, 0.0)) * 0.5;
    float g = hash(tile_grid_coord + vec2(1.0, 4.3));
    float b = hash(tile_grid_coord + vec2(2.5, 8.1));

    vColor = vec3(r, g, b);
    v_is_dirty_pass = float(u_is_dirty);
}