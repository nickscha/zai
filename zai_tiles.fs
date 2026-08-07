#version 330 core

in float v_is_dirty_pass;
in vec2 v_uv;
in vec3 v_worldPos;

out vec4 FragColor;

uniform vec3 iResolution;
uniform vec3 sunDir;
uniform vec3 iCamera;
uniform vec3 iViewDir;
uniform float visualization_mode; /* just for testing */
uniform sampler2D u_normalmap;

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

void main()
{
    vec3 normal = normalize(texture(u_normalmap, v_uv).rgb * 2.0 - 1.0);
    
    vec3 matRock  = vec3(0.12, 0.11, 0.10);
    vec3 matGrass = vec3(0.06, 0.10, 0.03);
    vec3 matSnow  = vec3(0.90, 0.92, 0.95);

    float slope = normal.y;
    float height = v_worldPos.y;

    vec3 mate = matRock;
    float grassBlend = smoothstep(0.6, 0.8, slope);
    mate = mix(mate, matGrass, grassBlend);

    float snowBlend = smoothstep(120.0, 180.0, height) * smoothstep(0.5, 0.8, slope);
    float frostBlend = smoothstep(220.0, 260.0, height);
    snowBlend = max(snowBlend, frostBlend);
    mate = mix(mate, matSnow, snowBlend);

    float dayAmount = clamp(sunDir.y * 0.5 + 0.5, 0.0, 1.0);
    float sunsetAmount = exp(-abs(sunDir.y) * 7.0);

    vec3 sDir = normalize(sunDir);
    vec3 mDir = normalize(-sunDir);
    
    float sunDif = max(dot(normal, sDir), 0.0);
    float moonDif = max(dot(normal, mDir), 0.0);

    vec3 sunLightCol = mix(vec3(1.30, 1.15, 0.90), vec3(1.0, 0.45, 0.20), sunsetAmount);
    vec3 moonLightCol = vec3(0.08, 0.12, 0.18); 

    vec3 zenith = mix(vec3(0.005, 0.010, 0.020), vec3(0.08, 0.30, 0.80), dayAmount);
    zenith = mix(zenith, vec3(0.70, 0.25, 0.35), sunsetAmount);

    float skyDome = smoothstep(-0.2, 0.2, normal.y);
    float bounce = max(dot(normal, normalize(vec3(-sDir.x, 0.0, -sDir.z))), 0.0);
    float cavityAO = smoothstep(-0.2, 0.8, normal.y);

    vec3 light = vec3(0.0);
    light += sunDif * sunLightCol;
    light += moonDif * moonLightCol;
    light += skyDome * zenith * 0.6 * cavityAO;
    light += bounce * zenith * 0.2; 

    vec3 col = mate * light;

    vec3 rayDir = normalize(v_worldPos - iCamera);
    float dist = length(v_worldPos - iCamera);
    
    vec3 fogCol = getFogColor(rayDir);
    float distFog = 1.0 - exp(-dist * 0.00015);
    float heightFog = exp(-v_worldPos.y * 0.015);
    float totalFog = clamp(distFog * heightFog, 0.0, 1.0);

    col = mix(col, fogCol, totalFog);

    col = pow(col, vec3(0.4545));
    
    /*
    if (v_is_dirty_pass > 0.5)
    {
        FragColor = vec4(1.0, 0.1, 0.1, 1.0);
    }
    else
    */

    if (visualization_mode < 0.5f) {
        FragColor = vec4(col, 1.0);
    } else if (visualization_mode < 1.5f) {
        FragColor = vec4(normal * 0.5 + 0.5, 1.0); /* map normal to 0 - 1 range */
    }
}