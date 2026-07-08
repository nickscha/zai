#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec3 u_tile_offset;
uniform mat4 u_vp;

out vec3 vColor; 

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main()
{
    vec2 world_pos = aPos + u_tile_offset.xy;
    gl_Position = u_vp * vec4(world_pos.x, 0.0, world_pos.y, 1.0);

    float r = hash(u_tile_offset.xy + vec2(0.0, 0.0));
    float g = hash(u_tile_offset.xy + vec2(1.0, 4.3));
    float b = hash(u_tile_offset.xy + vec2(2.5, 8.1));

    vColor = vec3(r, g, b);
}