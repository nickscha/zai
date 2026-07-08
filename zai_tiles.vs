#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aInstancePos;

uniform mat4 uVP;

void main()
{
    vec2 worldPos = aPos + aInstancePos;
    gl_Position = uVP * vec4(worldPos.x, 0.0, worldPos.y, 1.0);
}
