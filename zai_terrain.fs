#version 330 core

in float vHeight;
in vec3  vNormal;
in vec3  vWorldPos;

out vec4 FragColor;

void main()
{
    // Visualize normals: (-1 to 1) mapped to (0 to 1)
    vec3 normalColor = vNormal * 0.5 + 0.5;
    
    // Optional: Add some very basic diffuse lighting so it looks 3D
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(vNormal, lightDir), 0.0);
    vec3 terrainColor = vec3(0.3, 0.6, 0.3) * diff;

    // Mix between raw normal view and lit view for debugging
    FragColor = vec4(terrainColor / normalColor, 1.0);
}