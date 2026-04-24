#version 330 core

uniform vec3 iResolution;
uniform sampler2D tex_diffuse;
uniform sampler2D tex_normal;
uniform sampler2D tex_displacement;

in vec3  vNormal;
in vec3  vWorldPos;

out vec4 FragColor;

void main()
{
    vec3 normalColor = vNormal * 0.5 + 0.5;
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(vNormal, lightDir), 0.0);
    vec3 terrainColor = vec3(0.3, 0.6, 0.3) * diff;
    vec3 finalColor = terrainColor / normalColor;

    /* Preview Textures */
    {
        float size = 200.0;     // Size of each preview square
        float pad  = 10.0;      // Padding from screen edge and between squares
        
        // Calculate boundaries for the stack
        // x range is the same for all three
        float xMin = iResolution.x - (size + pad);
        float xMax = iResolution.x - pad;

        if (gl_FragCoord.x > xMin && gl_FragCoord.x < xMax)
        {
            // 1. Bottom slot: Diffuse
            float yMin1 = pad;
            float yMax1 = pad + size;
            
            // 2. Middle slot: Normal
            float yMin2 = yMax1 + pad;
            float yMax2 = yMin2 + size;
            
            // 3. Top slot: Displacement
            float yMin3 = yMax2 + pad;
            float yMax3 = yMin3 + size;

            if (gl_FragCoord.y > yMin1 && gl_FragCoord.y < yMax1) {
                vec2 uv = (gl_FragCoord.xy - vec2(xMin, yMin1)) / size;
                finalColor = texture(tex_diffuse, uv).rgb;
            }
            else if (gl_FragCoord.y > yMin2 && gl_FragCoord.y < yMax2) {
                vec2 uv = (gl_FragCoord.xy - vec2(xMin, yMin2)) / size;
                finalColor = texture(tex_normal, uv).rgb;
            }
            else if (gl_FragCoord.y > yMin3 && gl_FragCoord.y < yMax3) {
                vec2 uv = (gl_FragCoord.xy - vec2(xMin, yMin3)) / size;
                finalColor = texture(tex_displacement, uv).rgb;
            }
        }
    }

    FragColor = vec4(finalColor, 1.0);
}