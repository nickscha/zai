#version 330 core

uniform vec3 iResolution;
uniform vec3 iCamera;
uniform vec3 sunDir;
uniform sampler2D tex_diffuse;
uniform sampler2D tex_normal;
uniform sampler2D tex_displacement;

in vec3  vNormal;
in vec3  vWorldPos;
in float vDepth;

out vec4 FragColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

/* Matches the Skybox shader */
vec3 getFogColor(vec3 rd)
{
    float h = max(rd.y, 0.0);

    float dayAmount = clamp(sunDir.y * 0.5 + 0.5, 0.0, 1.0);
    float sunsetAmount = exp(-abs(sunDir.y) * 7.0);

    /* Colors */
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

    float horizonGlow = pow(1.0 - h, 5.0);
    sky += sunsetHorizon * horizonGlow * sunsetAmount * 0.5;

    float sunAmount = max(dot(rd, sunDir), 0.0);
    
    vec3 sunColor =
        mix(
            vec3(1.0, 0.95, 0.85),
            vec3(1.0, 0.45, 0.20),
            sunsetAmount
        );

    /* Moon */
    float nightAmount = 1.0 - dayAmount;
    vec3 moonDir = -sunDir;
    float moonAmount = max(dot(rd, moonDir), 0.0);
    float moonGlow = pow(moonAmount, 20.0);
    vec3 moonColor = vec3(0.8, 0.85, 1.0);

    sky += moonColor * moonGlow * 0.08 * nightAmount;

    /* Athmospheric Scattering */
    float mie = pow(sunAmount, 8.0);
    sky += sunColor * mie * 0.25;

    return sky;
}

void main() {
    vec3 normal = normalize(vNormal);
    
    float height = vWorldPos.y;

    vec3 lowColor  = vec3(0.18, 0.16, 0.12);
    vec3 highColor = vec3(0.62, 0.30, 0.26);
    float heightBlend = smoothstep(400.0, 800.0, height);
    vec3 terrainBase = mix(lowColor, highColor, heightBlend);

    /* Terrain noise */
    float detail = hash(floor(vWorldPos.xz * 2.0));
    terrainBase *= 0.97 + detail * 0.06;

    // Basic lighting setup
    float NdotL = max(dot(normal, sunDir), 0.0);
    float sunsetAmount = exp(-abs(sunDir.y) * 7.0);

    vec3 sunlightColor = mix(
        vec3(1.0, 0.95, 0.85),
        vec3(1.0, 0.45, 0.20),
        sunsetAmount
    );

    vec3 finalColor = terrainBase * sunlightColor * (NdotL * 0.9 + 0.1);

    vec3 viewVec = vWorldPos - iCamera;
    vec3 viewDir = normalize(viewVec);
    float dist = length(viewVec);

    float fogFactor = 1.0 - exp(-dist * 0.00003);
    float horizonScale = pow(1.0 - abs(viewDir.y), 4.0);
    
    fogFactor += horizonScale * 0.25 * fogFactor; 
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    vec3 fogColor = getFogColor(viewDir);
    
    finalColor = mix(finalColor, fogColor, fogFactor);
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    float noise = hash(gl_FragCoord.xy);
    finalColor += (noise - 0.5) / 255.0;

    float fakeAO = pow(normal.y, 1.5);
    finalColor *= mix(0.65, 1.0, fakeAO);

    FragColor = vec4(finalColor, 1.0);
}