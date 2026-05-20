#version 330 core

in vec2 vUV;

out vec4 FragColor;

uniform vec3 iResolution;
uniform float iTime;
uniform vec3 cameraPos;
uniform mat3 cameraBasis; /* right, up, forward */
uniform vec3 sunDir;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float hash3D(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

vec3 getSky(vec3 rd)
{
    float h = max(rd.y, 0.0);

    float dayAmount = clamp(sunDir.y * 0.5 + 0.5, 0.0, 1.0);
    float nightAmount = 1.0 - dayAmount;
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
    
    float sunAmount = max(dot(rd, sunDir), 0.0);
    float sunGlow   = pow(sunAmount, 32.0);
    float sunDisk   = pow(sunAmount, 2000.0);

    vec3 sunColor =
        mix(
            vec3(1.0, 0.95, 0.85),
            vec3(1.0, 0.45, 0.20),
            sunsetAmount
        );

    sky += sunColor * sunGlow * 0.6;
    sky += sunColor * sunDisk * 8.0;
    
    /* Moon */
    vec3 moonDir = -sunDir;

    float moonAmount = max(dot(rd, moonDir), 0.0);

    float moonDisk = pow(moonAmount, 1500.0);
    float moonGlow = pow(moonAmount, 20.0);
    vec3 moonColor = vec3(0.8, 0.85, 1.0);

    sky += moonColor * moonGlow * 0.08 * nightAmount;
    sky += moonColor * moonDisk * 2.5 * nightAmount;

    /* Athmospheric Scattering */
    float mie = pow(sunAmount, 8.0);
    sky += sunColor * mie * 0.25;

    /* Horizon glow */
    float horizonGlow = pow(1.0 - h, 8.0);
    sky += sunsetHorizon * horizonGlow * sunsetAmount * 0.5;

    /* Stars */
    float trueNightAmount = smoothstep(0.0, -0.2, sunDir.y);

    if (trueNightAmount > 0.001) {
        vec3 starZ = normalize(-sunDir); 
        vec3 starX = normalize(cross(vec3(0.0, 1.0, 0.0), starZ));
        vec3 starY = cross(starZ, starX);
        mat3 celestialRotation = mat3(starX, starY, starZ);
        
        vec3 rotatedRd = rd * celestialRotation;

        float gridScale = 500.0; 
        vec3 starGrid = floor(rotatedRd * gridScale); 
        
        float starHash = hash3D(starGrid);
        float starIntensity = 0.0;

        if (starHash > 0.998) {
            float normalizedHash = (starHash - 0.992) / (1.0 - 0.992);
            starIntensity = pow(normalizedHash, 2.0) * trueNightAmount;
        }

        if (starIntensity > 0.0) {

            vec3 cellCenter = normalize((starGrid + 0.5) / gridScale);
            float angleDist = 1.0 - dot(rotatedRd, cellCenter);

            float starSize = 0.000002; 
            starIntensity *= smoothstep(starSize, 0.0, angleDist);
            starIntensity *= smoothstep(0.0, 0.1, rd.y); 

            float skyLuminance = dot(sky, vec3(0.2126, 0.7152, 0.0722));
            starIntensity *= max(1.0 - skyLuminance * 4.0, 0.0); 

            sky += vec3(starIntensity * 0.9, starIntensity * 0.95, starIntensity * 1.0);
        }
    }
        
    return sky;
}

void main()
{
    vec2 uv = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    vec3 rd = normalize(cameraBasis * vec3(uv, 1.0));
    vec3 sky = getSky(rd);

    sky = pow(sky, vec3(1.0 / 2.2));

    float noise = hash(gl_FragCoord.xy);
    sky += (noise - 0.5) / 255.0;

    FragColor = vec4(sky, 1.0);
}
