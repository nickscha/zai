#version 330 core

uniform vec3 u_tile_offset;
uniform mat4 u_vp;
uniform float u_is_dirty;
uniform float u_tile_size;
uniform float u_grid_res;
uniform sampler2D u_heightmap;

out float v_is_dirty_pass;
out vec2 v_uv;
out vec3 v_worldPos;

void main()
{
    float v_x = float(gl_VertexID % int(u_grid_res));
    float v_z = float(gl_VertexID / int(u_grid_res));

    v_uv = vec2(v_x, v_z) / (u_grid_res - 1.0);

    vec2 local_pos = (vec2(v_x, v_z) / (u_grid_res - 1.0) - vec2(0.5)) * u_tile_size;
    vec2 world_pos = local_pos + (u_tile_offset.xy * u_tile_size);

    float height = texelFetch(u_heightmap, ivec2(v_x, v_z), 0).r;
    
    gl_Position     = u_vp * vec4(world_pos.x, height, world_pos.y, 1.0);
    v_is_dirty_pass = u_is_dirty;
    v_worldPos      = vec3(world_pos.x, height, world_pos.y);
}