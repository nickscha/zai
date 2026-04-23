#version 330 core
in vec3 vNormal;
out vec4 FragColor;

void main() {
    vec3 color = normalize(vNormal) * 0.5 + 0.5;
    FragColor = vec4(color, 1.0);
}