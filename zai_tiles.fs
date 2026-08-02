#version 330 core

in vec3 vColor;
in float v_is_dirty_pass;
in vec2 v_uv;

out vec4 FragColor;

uniform sampler2D u_normalmap;

void main()
{
    if (v_is_dirty_pass > 0.5)
    {
        FragColor = vec4(1.0, 0.1, 0.1, 1.0);
    }
    else
    {
        FragColor = vec4(vColor, 1.0);
    }

    vec3 normal = normalize(texture(u_normalmap, v_uv).rgb * 2.0 - 1.0);
    FragColor = vec4(normal * 0.5 + 0.5, 1.0); /* map normal to 0 - 1 range */
}