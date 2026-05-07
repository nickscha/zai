#version 330 core
layout(location=0) in vec2 uv;

uniform vec3  iCamera;
uniform float iBaseScale;
uniform mat4  MVP;

out vec3  vNormal;
out vec3  vWorldPos;
out float vDepth;

float hash(vec2 p)
{
    // IQ-style fractal hash
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

// Smooth interpolation curve (IQ classic)
vec2 fade(vec2 t)
{
    return t * t * (3.0 - 2.0 * t);
}

// 2D value noise
float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    // four corners
    float a = hash(i + vec2(0.0, 0.0));
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    // smooth interpolation
    vec2 u = fade(f);

    return mix(a, b, u.x)
         + (c - a) * u.y * (1.0 - u.x)
         + (d - b) * u.x * u.y;
}


float fbm(vec2 p)
{
    float value = 0.0;
    float amp = 0.5;

    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);

    for (int i = 0; i < 5; i++)
    {
        value += amp * valueNoise(p);
        p = m * p;
        amp *= 0.5;
    }

    return value;
}

float getHeightStable(vec2 anchor, vec2 local)
{   
    float h = fbm((anchor + local) * 0.001); 
    
    return h * 800.0;
}

void main() {
    int lod = gl_InstanceID;

    /* LOD inner ring discard */
    if (lod > 0)
    {   
        /* exact quarter point: 16/64 (grid is 65x65 so 64x64 quads */ 
        //const float inner = 0.28125; /* 18/64 = move cut line by 2 vertices */
        //const float outer = 0.71875; /* 46/64 = move cut line by 2 vertices from other side */

        const float inner = 0.265625;  // 34 / 128
        const float outer = 0.734375;  // 94 / 128

        if (uv.x > inner && uv.x < outer && uv.y > inner && uv.y < outer) 
        {
            gl_Position = vec4(0.0, 0.0, 2.0, 0.0); 
            return;
        }
    }

    float scale = iBaseScale * exp2(float(lod));
    float spacing = scale / iBaseScale;

    vec2 snappedCam = floor(iCamera.xz / (spacing * 2.0)) * (spacing * 2.0);

    vec2 localPos = (uv - 0.5) * scale;
    vec2 worldXZ = snappedCam + localPos;

    float height = getHeightStable(snappedCam, localPos);
    
    float hx = getHeightStable(snappedCam, localPos + vec2(spacing, 0.0));
    float hz = getHeightStable(snappedCam, localPos + vec2(0.0, spacing));

    vec3 N = normalize(vec3(height - hx, spacing, height - hz));

    vNormal = N;

    //float lodBias = float(10 - lod) * 0.1;
    //vWorldPos = vec3(worldXZ.x, height + lodBias, worldXZ.y);

    vWorldPos = vec3(worldXZ.x, height, worldXZ.y);

    gl_Position = MVP * vec4(vWorldPos, 1.0);
    vDepth = gl_Position.w;
}