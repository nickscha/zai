#version 330 core

uniform vec3 iResolution;
uniform vec3 iCamera;
uniform sampler2D tex_diffuse;
uniform sampler2D tex_normal;
uniform sampler2D tex_displacement;

in float vLod;
in float vHeight;
in vec3  vNormal;
in vec3  vWorldPos;

out vec4 FragColor;

void getTriplanarData(vec2 uv, vec3 viewDirTangent, out vec2 pUV, out vec3 nSample, out float ao) {
    float heightScale = 0.04; 
    float h = texture(tex_displacement, uv).r;
    
    pUV = uv + (viewDirTangent.xy * (h * heightScale));
    nSample = texture(tex_normal, pUV).rgb * 2.0 - 1.0;
    ao = texture(tex_displacement, pUV).r;
}

void main()
{
    vec3 geomNormal = normalize(vNormal);
    vec3 viewDir = normalize(iCamera - vWorldPos);
    float tiling = 0.02; 
    vec3 blend = abs(geomNormal);
    blend /= (blend.x + blend.y + blend.z);

    vec2 uvX = vWorldPos.zy * tiling;
    vec2 uvY = vWorldPos.xz * tiling;
    vec2 uvZ = vWorldPos.xy * tiling;

    vec2 pUVx, pUVy, pUVz;
    vec3 nX, nY, nZ;
    float aoX, aoY, aoZ;

    // Plane X (Side)
    getTriplanarData(uvX, viewDir.zyx, pUVx, nX, aoX);
    // Plane Y (Top - Main)
    getTriplanarData(uvY, viewDir.xzy, pUVy, nY, aoY);
    // Plane Z (Side)
    getTriplanarData(uvZ, viewDir.xyz, pUVz, nZ, aoZ);


    nX = vec3(nX.z, nX.y, nX.x);
    nY = vec3(nY.x, nY.z, nY.y);
    nZ = vec3(nZ.x, nZ.y, nZ.z);

    vec3 worldNormal = normalize(nX * blend.x + nY * blend.y + nZ * blend.z + geomNormal);

    // 4. Color & AO Blending
    vec3 colX = texture(tex_diffuse, pUVx).rgb;
    vec3 colY = texture(tex_diffuse, pUVy).rgb;
    vec3 colZ = texture(tex_diffuse, pUVz).rgb;
    
    vec3 rockColor = colX * blend.x + colY * blend.y + colZ * blend.z;
    float finalAO  = aoX * blend.x + aoY * blend.y + aoZ * blend.z;

    // 5. Lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(worldNormal, lightDir), 0.0);
    
    vec3 ambient = rockColor * 0.2 * finalAO;
    vec3 diffuse = rockColor * diff;
    
    vec3 finalColor = diffuse + ambient;

    // --- DEBUG PREVIEWS ---
    float size = 200.0;
    float pad  = 10.0;
    float xMin = iResolution.x - (size + pad);
    float xMax = iResolution.x - pad;

    if (gl_FragCoord.x > xMin && gl_FragCoord.x < xMax)
    {
        float yMin1 = pad; float yMax1 = pad + size;
        float yMin2 = yMax1 + pad; float yMax2 = yMin2 + size;
        float yMin3 = yMax2 + pad; float yMax3 = yMin3 + size;

        if (gl_FragCoord.y > yMin1 && gl_FragCoord.y < yMax1) {
            finalColor = texture(tex_diffuse, (gl_FragCoord.xy - vec2(xMin, yMin1)) / size).rgb;
        } else if (gl_FragCoord.y > yMin2 && gl_FragCoord.y < yMax2) {
            finalColor = texture(tex_normal, (gl_FragCoord.xy - vec2(xMin, yMin2)) / size).rgb;
        } else if (gl_FragCoord.y > yMin3 && gl_FragCoord.y < yMax3) {
            finalColor = texture(tex_displacement, (gl_FragCoord.xy - vec2(xMin, yMin3)) / size).rgb;
        }
    }

    FragColor = vec4(finalColor, 1.0);
}