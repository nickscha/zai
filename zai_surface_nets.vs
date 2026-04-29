#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 vNormal;
out vec3 vWorldPos;
out float vDepth;

uniform mat4 MVP;

void main() {
    vNormal = aNormal;
    vWorldPos = aPos;
    gl_Position = MVP * vec4(aPos, 1.0);
    vDepth = gl_Position.w;
}