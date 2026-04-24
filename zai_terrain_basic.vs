#version 330 core
layout(location=0) in vec2 uv;

uniform vec3  iCamera;
uniform float iBaseScale;
uniform mat4  MVP;

out vec3  vNormal;
out vec3  vWorldPos;

void main() {
    int lod = gl_InstanceID;

    /* LOD inner ring discard */
    if (lod > 0)
    {   
        /* exact quarter point: 16/64 (grid is 65x65 so 64x64 quads */ 
        const float inner = 0.28125; /* 18/64 = move cut line by 2 vertices */
        const float outer = 0.71875; /* 46/64 = move cut line by 2 vertices from other side */

        if (uv.x > inner && uv.x < outer && uv.y > inner && uv.y < outer) 
        {
            gl_Position = vec4(0.0, 0.0, 2.0, 0.0); 
            return;
        }
    }

    float scale = iBaseScale * exp2(float(lod));
    float spacing = scale / 64.0;
    vec2 snappedCam = floor(iCamera.xz / spacing) * spacing;
    vec2 localPos = (uv - 0.5) * scale;
    vec2 worldXZ = snappedCam + localPos;

    float height = 0.0;

    /* Set outputs */    
    vNormal = vec3(0.0, 1.0, 0.0);
    vWorldPos = vec3(worldXZ.x, height, worldXZ.y);
    gl_Position = MVP * vec4(vWorldPos, 1.0);
}