#version 330 core
layout(location=0) in vec2 uv;

uniform vec3  iResolution;
uniform vec3  iCamera;
uniform vec3  iViewDir; // camera forward normalized vector
uniform float iBaseScale;
uniform mat4  MVP;

out vec3  vNormal;
out vec3  vWorldPos;
out float vDepth;
out float vHeight;

vec2 hash( vec2 p ) {
    p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

vec3 noised( in vec2 p ) {
    vec2 i = floor( p );
    vec2 f = fract( p );
    vec2 u = f*f*f*(f*(f*6.0-15.0)+10.0);
    vec2 du = 30.0*f*f*(f*(f-2.0)+1.0);
    
    float a = hash(i + vec2(0,0)).x;
    float b = hash(i + vec2(1,0)).x;
    float c = hash(i + vec2(0,1)).x;
    float d = hash(i + vec2(1,1)).x;

    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k3 = a - b - c + d;

    return vec3(k0 + k1*u.x + k2*u.y + k3*u.x*u.y, 
            du * vec2(k1 + k3*u.y, k2 + k3*u.x));
}

vec3 fbm_9( vec2 p ) {
    float f = 0.0;
    vec2  d = vec2(0.0);
    float a = 0.5;
    mat2  m = mat2(1.6, 1.2, -1.2, 1.6);
    for( int i=0; i<9; i++ ) {
        vec3 n = noised(p);
        f += a * n.x;
        d += a * n.yz;
        a *= 0.5;
        p = m * p;
    }
    return vec3(f, d);
}

vec3 terrainMap( vec2 p ) {
    vec3 n = fbm_9( p / 2000.0 + vec2(1.0, -2.0) );
    
    float e = n.x;
    vec2 de = n.yz / 2000.0; 

    float h = 600.0 * e + 600.0;
    vec2 dh = 600.0 * de;

    float cliff_input = h; 
    float t = clamp((cliff_input - 552.0) / (594.0 - 552.0), 0.0, 1.0);
    float cliff_weight = t * t * (3.0 - 2.0 * t);
    float d_cliff_weight = 6.0 * t * (1.0 - t) / (594.0 - 552.0);

    float finalH = h + 90.0 * cliff_weight;
    vec2 finalDH = dh + 90.0 * d_cliff_weight * dh;

    return vec3(finalH, finalDH);
}

void main() {
    int lod = gl_InstanceID;
    
    float scale = iBaseScale * exp2(float(lod));
    float spacing = scale / iBaseScale;

    vec3 camTerrain = terrainMap(iCamera.xz);
    float heightAboveGround = max(0.0, iCamera.y - camTerrain.x);

    float biasFactor = 0.95; 
    float biasWeight = clamp(1.0 - (heightAboveGround / 600.0), 0.0, 1.0);   
    vec2 forwardBias = iViewDir.xz * (scale * biasFactor) * biasWeight;

    vec2 posToSnap = iCamera.xz + forwardBias;
    vec2 snappedCam = floor(posToSnap / (spacing * 2.0)) * (spacing * 2.0);

    vec2 localPos = (uv - 0.5) * scale;
    vec2 worldXZ = snappedCam + localPos;

    if (lod > 0) {
        float prevScale = iBaseScale * exp2(float(lod - 1));
        
        vec2 prevForwardBias = iViewDir.xz * (prevScale * biasFactor) * biasWeight;
        vec2 prevSnappedCam = floor((iCamera.xz + prevForwardBias) / (spacing)) * (spacing);
        
        vec2 diff = abs(worldXZ - prevSnappedCam);
        float innerHalfSize = prevScale * 0.5;

        if (diff.x < innerHalfSize - spacing && diff.y < innerHalfSize - spacing) {
            gl_Position = vec4(0.0, 0.0, 2.0, 0.0);
            return;
        }
    }

    /* Morphing */
    vec2 alpha = abs(uv - 0.5) * 2.0; 
    float maxAlpha = max(alpha.x, alpha.y);
    float transition = smoothstep(0.85, 0.98, maxAlpha);

    if (transition > 0.0) {
        float coarseSpacing = spacing * 2.0;
        vec2 gridSnappedXZ = worldXZ - mod(worldXZ, coarseSpacing);
        worldXZ = mix(worldXZ, gridSnappedXZ, transition);
    }

    vec3 terrain = terrainMap(worldXZ);

    vWorldPos = vec3(worldXZ.x, terrain.x, worldXZ.y);
    vNormal = normalize(vec3(-terrain.y, 1.0, -terrain.z));
    vHeight = terrain.x;
    gl_Position = MVP * vec4(vWorldPos, 1.0);
    vDepth = gl_Position.w;
}