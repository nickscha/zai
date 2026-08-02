#version 330 core

in vec3 vColor;
in float v_is_dirty_pass;
out vec4 FragColor;

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
}