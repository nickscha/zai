#version 330 core

out vec4 FragColor;

uniform vec3  iResolution;
uniform float iTime;
uniform usampler3D uBrickMap;
uniform sampler3D  uAtlas;
uniform sampler3D uMaterial;
uniform sampler1D uPalette;

uniform vec3  uBrickMapDim;
uniform ivec3 uAtlasBrickDim;
uniform vec3  uInvAtlasSize;
uniform vec3  uGridStart;
uniform float uCellSize;
uniform float uTruncation;
uniform vec3  uInvCellSize;

uniform vec3  camera_position;
uniform vec3  camera_forward;
uniform vec3  camera_right;
uniform vec3  camera_up;
uniform vec3  camera_forward_scaled;

const float fBRICK_SIZE = 8.0;
const float fPHYSICAL_BRICK_SIZE = 10.0;
const float EPS = 0.01;
const float INV_256 = 1.0 / 256.0;

vec3 debugColor(ivec3 p) {
    uvec3 v = uvec3(p);
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    return vec3(v & 255u) / 255.0;
}

void drawAtlasDebug(inout vec3 col, vec2 fragCoord) {
    float size = iResolution.y * 0.4; // Debug window is 40% of screen height
    vec2 padding = vec2(10.0);
    vec2 bMin = iResolution.xy - vec2(size) - padding;
    vec2 bMax = iResolution.xy - padding;

    if (fragCoord.x > bMin.x && fragCoord.x < bMax.x && 
        fragCoord.y > bMin.y && fragCoord.y < bMax.y) {
        
        vec2 uv = (fragCoord - bMin) / size;
        float zSlice = mod(iTime * 0.5, 1.0);
        float val = textureLod(uAtlas, vec3(uv, zSlice), 0.0).r;
        vec3 debugCol = vec3(val);
        float border = 2.0;
        
        if (fragCoord.x < bMin.x + border || fragCoord.x > bMax.x - border ||
            fragCoord.y < bMin.y + border || fragCoord.y > bMax.y - border) {
            debugCol = vec3(0.1, 0.1, 0.1);
        }

        col = debugCol;
    }
}

void main()
{
    vec2 uv = (2.0 * gl_FragCoord.xy - iResolution.xy) / iResolution.y;
    vec3 ro = camera_position; 
    vec3 rd = normalize(uv.x * camera_right + uv.y * camera_up + camera_forward_scaled); 

    vec3 invRd = 1.0 / rd;
    vec3 t0 = (uGridStart - ro) * invRd;
    vec3 t1 = ((uGridStart + uBrickMapDim * uCellSize) - ro) * invRd;
    
    float tNear = max(max(min(t0.x, t1.x), min(t0.y, t1.y)), min(t0.z, t1.z));
    float tFar  = min(min(max(t0.x, t1.x), max(t0.y, t1.y)), max(t0.z, t1.z));

    vec3 col = vec3(0.4, 0.75, 1.0) - 0.7 * rd.y; // Sky

    if (tNear < tFar && tFar > 0.0) {
        float t = max(0.0, tNear) + EPS;
        float hitT = -1.0;
        uint hitStored = 0u;
        vec3 hitAtlasOff = vec3(0.0);

        // DDA Setup
        vec3 gridP = (ro + rd * t - uGridStart) * uInvCellSize;
        ivec3 brickCoord = ivec3(floor(gridP / fBRICK_SIZE));
        
        vec3 rdSign = sign(rd);
        ivec3 iRdSign = ivec3(rdSign);
        vec3 uvDir = rd * uInvCellSize * uInvAtlasSize;
        vec3 tDelta = abs((fBRICK_SIZE * uCellSize) * invRd);
        vec3 tMax = ((vec3(brickCoord) + max(rdSign, 0.0)) * fBRICK_SIZE - gridP) * uCellSize * invRd + t;

        float fBricksPerRow = float(uAtlasBrickDim.x);
        float invBricksPerRow = 1.0 / fBricksPerRow;
        float coneFactor = 1.5 / iResolution.y;

        // Brick Traversal (DDA)
        for(int i = 0; i < 48; i++) {

            hitStored = texelFetch(uBrickMap, brickCoord, 0).r;

            if (hitStored == 65535u) {
                hitT = t; break;
            } 
            else if (hitStored > 0u) {
                float fLinear = float(hitStored - 1u);
                vec2 offset;
                offset.y = floor(fLinear * invBricksPerRow);
                offset.x = fLinear - (offset.y * fBricksPerRow);
                hitAtlasOff = (vec3(offset * fPHYSICAL_BRICK_SIZE, 0.0) + 1.0) * uInvAtlasSize;

                float localT = t;
                float brickExitT = min(min(tMax.x, tMax.y), tMax.z);
                
                vec3 pStartGrid = (ro + rd * localT - uGridStart) * uInvCellSize;
                vec3 uvPos = hitAtlasOff - vec3(brickCoord * 8) * uInvAtlasSize + pStartGrid * uInvAtlasSize;
              
                 
                // Inner Loop: Sphere Tracing
                for(int j = 0; j < 32; j++) {
                    float d = textureLod(uAtlas, uvPos, 0.0).r * uTruncation;
                    float currentEps = EPS + (localT * coneFactor); 
                    
                    if (d < currentEps) { hitT = localT; break; }
                    
                    localT += d;
                    uvPos += uvDir * d;
                    if (localT > brickExitT) break;
                }
                if (hitT > 0.0) break;
            }

            if (tMax.x < tMax.y) {
                if (tMax.x < tMax.z) { t = tMax.x; tMax.x += tDelta.x; brickCoord.x += iRdSign.x; }
                else                 { t = tMax.z; tMax.z += tDelta.z; brickCoord.z += iRdSign.z; }
            } else {
                if (tMax.y < tMax.z) { t = tMax.y; tMax.y += tDelta.y; brickCoord.y += iRdSign.y; }
                else                 { t = tMax.z; tMax.z += tDelta.z; brickCoord.z += iRdSign.z; }
            }

            if (t > tFar) break;
        }

        if (hitT > 0.0) {
            vec3 pos = ro + rd * hitT;
            vec3 gP = (pos - uGridStart) * uInvCellSize;
            
            vec3 uvBase = hitAtlasOff - vec3(brickCoord * 8) * uInvAtlasSize;
            vec3 baseUV = uvBase + gP * uInvAtlasSize;

            float e = 0.1; 
            vec3 ee = vec3(e) * uInvAtlasSize; 
            vec2 k = vec2(1.0, -1.0);            

            vec3 normal = normalize(
                k.xyy * textureLod(uAtlas, baseUV + k.xyy * ee, 0.0).r +
                k.yyx * textureLod(uAtlas, baseUV + k.yyx * ee, 0.0).r +
                k.yxy * textureLod(uAtlas, baseUV + k.yxy * ee, 0.0).r +
                k.xxx * textureLod(uAtlas, baseUV + k.xxx * ee, 0.0).r
            );

            float matIDNormalized = textureLod(uMaterial, baseUV, 0.0).r;
            float paletteCoord = (matIDNormalized * 255.0 + 0.5) * INV_256;
            vec3 material = texture(uPalette, paletteCoord).rgb;

            float diffuse = clamp(dot(normal, normalize(vec3(0.7, 0.9, 0.3))), 0.0, 1.0);
            vec3 ambient  = vec3(0.2, 0.3, 0.4);
            vec3 sun      = vec3(0.8, 0.7, 0.5);

            //col = material * (ambient + diffuse * sun);
            col = ambient + diffuse * sun * normal;
            //col = ambient + diffuse * sun;

             /* 
            ivec3 voxelCoord = ivec3(floor((pos - uGridStart) * uInvCellSize));
            vec3 brick_color = debugColor(brickCoord);
            vec3 voxel_color = debugColor(voxelCoord);
            col = ambient + diffuse * sun * brick_color * (0.6 + 0.3 * voxel_color);
            */
        }
    }

    /*drawAtlasDebug(col, gl_FragCoord.xy);*/

    FragColor = vec4(pow(col, vec3(0.4545)), 1.0);
}