#version 330 core

in float v_is_dirty_pass;
in vec2 v_uv;
in vec3 v_worldPos;

out vec4 FragColor;

uniform vec3 iResolution;
uniform float iTime;
uniform vec3 sunDir;
uniform vec3 iCamera;
uniform vec3 iViewDir;
uniform float visualization_mode; /* just for testing */
uniform sampler2D u_normalmap;

float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise2D(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    f = f * f * (3.0 - 2.0 * f);

    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

vec3 getFogColor(vec3 rd)
{
    float h = max(rd.y, 0.0);

    float dayAmount = clamp(sunDir.y * 0.5 + 0.5, 0.0, 1.0);
    float nightAmount = 1.0 - dayAmount;
    float sunsetAmount = exp(-abs(sunDir.y) * 7.0);

    /* Colors from your Sky Shader */
    vec3 dayZenith     = vec3(0.08, 0.30, 0.80);
    vec3 dayHorizon    = vec3(0.70, 0.85, 1.00);
    vec3 sunsetZenith  = vec3(0.70, 0.25, 0.35);
    vec3 sunsetHorizon = vec3(1.00, 0.45, 0.10);
    vec3 nightZenith   = vec3(0.005, 0.010, 0.020);
    vec3 nightHorizon  = vec3(0.020, 0.030, 0.050);

    vec3 zenith  = mix(nightZenith, dayZenith, dayAmount);
    vec3 horizon = mix(nightHorizon, dayHorizon, dayAmount);

    zenith  = mix(zenith, sunsetZenith, sunsetAmount);
    horizon = mix(horizon, sunsetHorizon, sunsetAmount);

    vec3 sky = mix(horizon, zenith, pow(h, 0.35));
    
    float sunAmount = max(dot(rd, sunDir), 0.0);
    float sunGlow   = pow(sunAmount, 32.0);

    vec3 sunColor = mix(
        vec3(1.0, 0.95, 0.85),
        vec3(1.0, 0.45, 0.20),
        sunsetAmount
    );

    sky += sunColor * sunGlow * 0.6;
    
    vec3 moonDir = -sunDir;
    float moonAmount = max(dot(rd, moonDir), 0.0);
    float moonGlow = pow(moonAmount, 20.0);
    vec3 moonColor = vec3(0.8, 0.85, 1.0);

    sky += moonColor * moonGlow * 0.08 * nightAmount;

    float mie = pow(sunAmount, 8.0);
    sky += sunColor * mie * 0.25;

    float horizonGlow = pow(1.0 - h, 8.0);
    sky += sunsetHorizon * horizonGlow * sunsetAmount * 0.5;

    return sky;
}

float cloudHash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float cloudValueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(cloudHash(i + vec2(0.0, 0.0)), cloudHash(i + vec2(1.0, 0.0)), u.x),
        mix(cloudHash(i + vec2(0.0, 1.0)), cloudHash(i + vec2(1.0, 1.0)), u.x), 
        u.y
    );
}

float getCloudShadow(vec3 pos, vec3 sun_dir, float time) {
    if (sun_dir.y < 0.001) return 1.0;

    float cloudHeight = 1200.0;
    float t = (cloudHeight - pos.y) / sun_dir.y;
    
    if (t < 0.0) return 1.0; 

    vec3 cloudPos = pos + sun_dir * t;

    vec2 windDir2D = normalize(vec2(1.0, 1.0)); 
    float windSpeed = 0.02;
    vec2 windOffset = windDir2D * time * windSpeed;
    
    vec2 uv = cloudPos.xz * 0.0003 + windOffset;

    float density = 0.0;
    float a = 0.5;
    mat2 rot = mat2(0.80, 0.60, -0.60, 0.80);
    
    for (int i = 0; i < 5; i++) {
        density += a * cloudValueNoise(uv);
        uv = rot * uv * 2.02;
        a *= 0.5;
    }

    density = smoothstep(0.38, 0.75, density);

    //return mix(1.0, 0.05, density); 

    // sharp shadow falloff
    float shadowMask = smoothstep(0.0, 0.4, density);
    return mix(1.0, 0.1, shadowMask);
}

void main()
{
    vec3 normal = texture(u_normalmap, v_uv).rgb * 2.0 - 1.0;
    
    vec3 matGrass = vec3(0.06, 0.10, 0.03);
    vec3 matSnow  = vec3(0.90, 0.92, 0.95);

    float slope = normal.y;
    float height = v_worldPos.y;

    float rockNoise = noise2D(v_worldPos.xz * 0.04);
    vec3 rockDark  = vec3(0.09, 0.085, 0.08);
    vec3 rockLight = vec3(0.16, 0.15, 0.14);
    vec3 matRock = mix(rockDark, rockLight, rockNoise);

    vec3 mate = matRock;
    float grassBlend = smoothstep(0.6, 0.8, slope);
    mate = mix(mate, matGrass, grassBlend);

    float snowBlend = smoothstep(120.0, 180.0, height) * smoothstep(0.5, 0.8, slope);
    float frostBlend = smoothstep(220.0, 260.0, height);
    snowBlend = max(snowBlend, frostBlend);
    mate = mix(mate, matSnow, snowBlend);

    float detail = noise2D(v_worldPos.xz * 0.025);
    mate *= mix(0.85, 1.15, detail);

    float dayAmount = clamp(sunDir.y * 0.5 + 0.5, 0.0, 1.0);
    float sunsetAmount = exp(-abs(sunDir.y) * 7.0);

    vec3 sDir = normalize(sunDir);
    vec3 mDir = normalize(-sunDir);

    float wrap = 0.2; 
    float sunDif = max(dot(normal, sDir) + wrap, 0.0) / (1.0 + wrap);
    sunDif = pow(sunDif, 1.5); 
    
    float moonDif = max(dot(normal, mDir), 0.0);

    // make sun facing material sligthly warmer
    float sunFacing = pow(sunDif, 2.0);
    mate *= 1.0 + sunFacing * 0.08;

    vec3 sunLightCol = mix(vec3(1.30, 1.15, 0.90), vec3(1.0, 0.45, 0.20), sunsetAmount);
    vec3 moonLightCol = vec3(0.08, 0.12, 0.18); 

    vec3 zenith = mix(vec3(0.005, 0.010, 0.020), vec3(0.08, 0.30, 0.80), dayAmount);
    zenith = mix(zenith, vec3(0.70, 0.25, 0.35), sunsetAmount);

    float skyDome = smoothstep(-0.2, 0.2, normal.y);
    float bounce = max(dot(normal, normalize(vec3(-sDir.x, 0.0, -sDir.z))), 0.0);
    float cavityAO = smoothstep(-0.2, 0.8, normal.y);

    float cloudShadow = getCloudShadow(v_worldPos, sDir, iTime);

    vec3 light = vec3(0.0);

    light += sunDif * sunLightCol * cloudShadow; 
    light += moonDif * moonLightCol;
    light += skyDome * zenith * 0.6 * cavityAO;
    light += bounce * zenith * 0.2;

    // make shadow area more deeper and cooler
    //float ambientDrop = mix(0.6, 1.0, cloudShadow); 
    //light += skyDome * zenith * 0.6 * cavityAO * ambientDrop;
    //light += bounce * zenith * 0.2 * ambientDrop;

    /* ground bounce coloring
    float up = max(normal.y, 0.0);
    float down = max(-normal.y, 0.0);
    vec3 skyLight = mix(vec3(0.03, 0.035, 0.04), zenith, up);
    vec3 groundBounce = vec3(0.025, 0.018, 0.012) * down;
    light += skyLight * 0.6;
    light += groundBounce;
    */

    vec3 col = mate * light;

    vec3 rayDir = normalize(v_worldPos - iCamera);
    float dist = length(v_worldPos - iCamera);
    
    vec3 fogCol = getFogColor(rayDir);
    float distFog = 1.0 - exp(-dist * 0.00015);
    float heightFog = exp(-v_worldPos.y * 0.015);
    float totalFog = clamp(distFog * heightFog, 0.0, 1.0);

    col = mix(col, fogCol, totalFog);

    // far terrain less saturated
    float distant = smoothstep(100.0, 500.0, dist);
    float luminance =  dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(col, vec3(luminance), distant * 0.15);

    col = pow(col, vec3(0.4545)); // Gamma Correction

    //col = vec3(cloudShadow);

    if (visualization_mode < 0.5f) {
        FragColor = vec4(col, 1.0);
    } else if (visualization_mode < 1.5f) {
        FragColor = vec4(normal * 0.5 + 0.5, 1.0); /* map normal to 0 - 1 range */
    }  else if (visualization_mode < 2.5f) {
        FragColor = v_is_dirty_pass > 0.5 ? vec4(1.0, 0.1, 0.1, 1.0) : vec4(col, 1.0);
    }
}