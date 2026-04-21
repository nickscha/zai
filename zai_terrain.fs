#version 330 core

in float vLod;
in float vHeight;
in vec3  vNormal;
in vec3  vTangent;
in vec3  vBitangent;
in vec3  vWorldPos;

out vec4 FragColor;

void main()
{
    mat3 TBN = mat3(
        normalize(vTangent),
        normalize(vBitangent),
        normalize(vNormal)
    );

    vec3 normalColor = vNormal * 0.5 + 0.5;
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(vNormal, lightDir), 0.0);
    vec3 terrainColor = vec3(0.3, 0.6, 0.3) * diff;
    vec3 finalColor = terrainColor / normalColor;

    FragColor = vec4(finalColor, 1.0);
}