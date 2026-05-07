#version 330 core

uniform vec3 iResolution;
uniform vec3  iCamera;
uniform sampler2D tex_diffuse;
uniform sampler2D tex_normal;
uniform sampler2D tex_displacement;

in vec3  vNormal;
in vec3  vWorldPos;
in float vDepth;

out vec4 FragColor;

void main() {
    FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
}