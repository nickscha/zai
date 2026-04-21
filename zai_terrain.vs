#version 330 core
layout(location=0) in vec2 uv;

uniform vec3  iCamera;
uniform float iBaseScale;
uniform mat4  MVP;

out float vHeight;
out vec3  vNormal;
out vec3  vWorldPos;

/* configuration */
#define TERRAIN_FREQ 2000.0   // Higher = larger features (stretched out)
#define TERRAIN_HEIGHT 1200.0 // Vertical scale factor
#define TERRAIN_OFFSET 400.0  // Vertical shift (base altitude)
#define CLIFF_STRENGTH 120.0  // Height of the cliff step
#define CLIFF_MIN 152.0       // Start height of the cliff
#define CLIFF_MAX 294.0       // End height of the cliff

float hash1( vec2 p ) 
{
    p  = 50.0 * fract( p * 0.3183099 );
    return fract( p.x * p.y * (p.x + p.y) );
}

/* value noise with derivatives */
vec3 noised( in vec2 x )
{
    vec2 p = floor(x);
    vec2 w = fract(x);

    vec2 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    vec2 du = 30.0*w*w*(w*(w-2.0)+1.0); 
    
    float a = hash1(p + vec2(0,0));
    float b = hash1(p + vec2(1,0));
    float c = hash1(p + vec2(0,1));
    float d = hash1(p + vec2(1,1));

    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k3 = a - b - c + d;

    float val = k0 + k1*u.x + k2*u.y + k3*u.x*u.y;
    vec2 der = du * vec2(k1 + k3*u.y, k2 + k3*u.x);
    
    return vec3(-1.0 + 2.0 * val, 2.0 * der); 
}

const mat2 m2 = mat2(0.80, 0.60, -0.60, 0.80);

vec3 fbm_9d( in vec2 x ) 
{
    float f = 1.9;
    float s = 0.55;
    float a = 0.0;
    float b = 0.5;
    vec2  d = vec2(0.0); 

    for(int i=0; i<9; ++i) 
    {

        vec3 n = noised(x);
        a += b * n.x;          
        d += b * m2 * n.yz;      
        b *= s;
        x = f * m2 * x;

    }

    return vec3(a, d);
}

void main() {
    int lod = gl_InstanceID;

    /* Fix Gaps */
    if (lod > 0)
    {
        float inner = 0.28125; 
        float outer = 0.71875; 

        if (uv.x > inner && uv.x < outer && uv.y > inner && uv.y < outer) 
        {
            gl_Position = vec4(0.0, 0.0, 2.0, 0.0); return;
        }
    }

    float scale = iBaseScale * exp2(float(lod));
    float spacing = scale / 64.0;  /* 64.0; */ 
    vec2 worldXZ = iCamera.xz + (uv - 0.5) * scale;
    

    // --- MORPH LOGIC ---
    vec2 fractal_uv = abs(uv - 0.5) * 2.0;
    float max_dist = max(fractal_uv.x, fractal_uv.y);
    // Increased the morph range slightly (0.6 to 1.0) for a softer transition
    float morph = smoothstep(0.6, 1.0, max_dist);

    // Grid Snapping
    vec2 snapped_fine = floor(worldXZ / spacing) * spacing;
    
    float next_spacing = spacing * 2.0;
    vec2 snapped_coarse = floor(worldXZ / next_spacing) * next_spacing;

    // --- 1. MORPH THE COORDINATES ---
    // This is the "secret sauce". We slide the vertex position itself.
    vec2 finalXZ = mix(snapped_fine, snapped_coarse, morph);

    // --- 2. ANALYTIC HEIGHT & NORMAL ---
    // We sample heights at the SNAPPED positions to keep them locked to the grid
    vec3 res_fine = fbm_9d(snapped_fine / TERRAIN_FREQ + vec2(1.0, -2.0));
    float h_fine = res_fine.x * TERRAIN_HEIGHT + TERRAIN_OFFSET;
    h_fine += CLIFF_STRENGTH * smoothstep(CLIFF_MIN, CLIFF_MAX, h_fine);
    
    vec2 grad_fine = res_fine.yz * (TERRAIN_HEIGHT / TERRAIN_FREQ);
    
    // Always calculate coarse for the blend to ensure no jumps
    vec3 res_coarse = fbm_9d(snapped_coarse / TERRAIN_FREQ + vec2(1.0, -2.0));
    float h_coarse = res_coarse.x * TERRAIN_HEIGHT + TERRAIN_OFFSET;
    h_coarse += CLIFF_STRENGTH * smoothstep(CLIFF_MIN, CLIFF_MAX, h_coarse);
    vec2 grad_coarse = res_coarse.yz * (TERRAIN_HEIGHT / TERRAIN_FREQ);

    /* blend */
    vec2 grad = mix(grad_fine, grad_coarse, morph);
    
    vHeight = mix(h_fine, h_coarse, morph);
    vNormal = normalize(vec3(-grad.x, 1.0, -grad.y));
    vWorldPos = vec4(finalXZ.x, vHeight, finalXZ.y, 1.0).xyz;
    gl_Position = MVP * vec4(vWorldPos, 1.0);
}