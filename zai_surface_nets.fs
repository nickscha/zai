#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in float vDepth;
out vec4 FragColor;

uniform vec3 iResolution;

vec3 aces_tonemap(vec3 x) {
    return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14), 0.0, 1.0);
}

void main() {
    vec3 norm = normalize(vNormal);
    vec3 rd = normalize(vWorldPos);
    vec3 viewDir = -rd;
    
    float camY = 15.0; 
    vec3 ro = vec3(0.0, camY, 0.0); 

    vec3 tx = vec3(0.5, 0.2, 0.1); 
    vec3 gr = mix(vec3(1.0), vec3(0.8, 1.3, 0.2), smoothstep(0.5, 1.0, norm.y)); 
    vec3 baseColor = mix(tx, tx * gr, smoothstep(0.7, 1.0, norm.y));

    vec3 lp = vec3(80.0, 40.0, 120.0); 
    vec3 ld = normalize(lp - vWorldPos);
    float lDist = length(lp - vWorldPos);

    float atten = 3.0 / (1.0 + lDist * 0.005 + lDist * lDist * 0.00005);
    float diff = max(dot(norm, ld), 0.0);
    float spec = pow(max(dot(reflect(-ld, norm), rd), 0.0), 64.0);
    
    float upness = clamp(0.5 + 0.5 * norm.y, 0.0, 1.0);
    float ao = mix(0.4, 1.0, upness); 
    
    vec3 sceneCol = baseColor * (diff + ao * 0.5 + vec3(1.0, 0.7, 0.5) * spec);
    sceneCol *= atten;

    float c = 0.08; // How fast the fog thins as you go up
    float b = 0.02; // Overall thickness
    
    float fogAmount = (b/c) * exp(-ro.y*c) * (1.0 - exp(-vDepth*rd.y*c)) / rd.y;
    fogAmount = clamp(fogAmount, 0.0, 1.0);

    float sun = max(dot(rd, ld), 0.0);
    vec3 fogCol = mix(vec3(0.4, 0.5, 0.6), vec3(1.0, 0.9, 0.7), pow(sun, 4.0));
    
    sceneCol = mix(sceneCol, fogCol, fogAmount);

    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    sceneCol *= pow(16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y), 0.125) * 0.75 + 0.25;
    sceneCol = aces_tonemap(sceneCol);
    sceneCol = sqrt(clamp(sceneCol, 0.0, 1.0));

    FragColor = vec4(sceneCol, 1.0);
}