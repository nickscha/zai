#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec3 u_tile_offset;
uniform mat4 u_vp;
uniform int u_is_dirty;

out vec3 vColor;
out float v_is_dirty_pass;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main()
{
    vec2 world_pos = aPos + u_tile_offset.xy;
    gl_Position = u_vp * vec4(world_pos.x, 0.0, world_pos.y, 1.0);

    vec2 tile_grid_coord = floor(u_tile_offset.xy + vec2(0.5));
    float r = hash(tile_grid_coord + vec2(0.0, 0.0)) * 0.5;
    float g = hash(tile_grid_coord + vec2(1.0, 4.3));
    float b = hash(tile_grid_coord + vec2(2.5, 8.1));

    vColor = vec3(r, g, b);
    v_is_dirty_pass = float(u_is_dirty);
}