layout (points) in;
layout (points, max_vertices = 1) out;

in vec2 v_gridPos[];

uniform sampler2D uWaterMask;
uniform sampler2D uShadowMap;
uniform mat4 uVPLight;
uniform mat4 uInvVPLight;
uniform mat4 uVPCamera;

uniform vec3 uCamPos;
uniform vec3 uLightPos;

uniform float uAttenuation;
uniform float uIntensity;
uniform float uA;
uniform float uB;
uniform float uLightFar;
uniform float uFloorOffset;

uniform float uTime;


const float IOR_AIR   = 1.0;
const float IOR_WATER = 1.33;
const float ETA       = IOR_AIR / IOR_WATER;
// const float FLOOR_OFFSET = 0.15;

out float vIntensity;
out vec4 vClipPos;


vec3 getWaterNormal(vec2 uv, vec3 posCenter)
{
    vec2 off = vec2(0.01, 0.01);

    vec3 tX;
    vec4 rawRight = texture(uWaterMask, uv + vec2(off.x, 0.0));
    vec4 rawLeft  = texture(uWaterMask, uv - vec2(off.x, 0.0));

    if (rawRight.a > 0.9 && distance(rawRight.rgb, posCenter) > 0.001)
        tX = rawRight.rgb - posCenter;
    else if (rawLeft.a > 0.9 && distance(rawLeft.rgb, posCenter) > 0.001)
        tX = posCenter - rawLeft.rgb;
    else
        tX = vec3(1.0, 0.0, 0.0);

    vec3 tZ;
    vec4 rawUp   = texture(uWaterMask, uv + vec2(0.0, off.y));
    vec4 rawDown = texture(uWaterMask, uv - vec2(0.0, off.y));

    if (rawUp.a > 0.9 && distance(rawUp.rgb, posCenter) > 0.001)
        tZ = rawUp.rgb - posCenter;
    else if (rawDown.a > 0.9 && distance(rawDown.rgb, posCenter) > 0.001)
        tZ = posCenter - rawDown.rgb;
    else
        tZ = vec3(0.0, 0.0, 1.0);

    vec3 n = normalize(cross(tX, tZ));
    if(n.y < 0.0) n = -n;
    return n;
}


vec3 reconstructFromShadow(vec2 uv, float depth) {
    vec4 lightClip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = uInvVPLight * lightClip;
    return worldPos.xyz / worldPos.w;
}

vec3 findFloor(vec3 position, vec3 direction){
    const int MAX_STEPS = 32;
    const int MAX_BISECT_STEPS = 16; 
    const float DEPTH_EPS = 0.0001;

    vec3 dir = normalize(direction);

    vec4 startClip = uVPLight * vec4(position, 1.0);
    vec3 startNDC = startClip.xyz / startClip.w;
    vec2 startUV = startNDC.xy * 0.5 + 0.5;

    if(startUV.x < 0.0 || startUV.x > 1.0 || startUV.y < 0.0 || startUV.y > 1.0)
        return position;

    float startRayDepth = startNDC.z * 0.5 + 0.5;
    float startDepth = texture(uShadowMap, startUV).r;
    float prevDiff = startDepth - startRayDepth;

    if(abs(prevDiff) < DEPTH_EPS){
        vec3 floorPoint = reconstructFromShadow(startUV, startDepth);
        return floorPoint - dir * uFloorOffset;
    }

    float maxDistance = uLightFar;
    float stepSize = maxDistance / float(MAX_STEPS);

    float prevDistance = 0.0;
    vec2 prevUV = startUV;
    float prevDepth = startDepth;

    for(int i = 1; i <= MAX_STEPS; ++i){
        float currentDistance = stepSize * float(i);
        vec3 currentPoint = position + dir * currentDistance;
        vec4 currentClip = uVPLight * vec4(currentPoint, 1.0);
        vec3 currentNDC = currentClip.xyz / currentClip.w;
        vec2 currentUV = currentNDC.xy * 0.5 + 0.5;

        if(currentUV.x < 0.0 || currentUV.x > 1.0 || currentUV.y < 0.0 || currentUV.y > 1.0)
            break;

        float currentRayDepth = currentNDC.z * 0.5 + 0.5;
        float currentDepth = texture(uShadowMap, currentUV).r;
        float currentDiff = currentDepth - currentRayDepth;

        if(abs(currentDiff) < DEPTH_EPS){
            vec3 floorPoint = reconstructFromShadow(currentUV, currentDepth);
            return floorPoint - dir * uFloorOffset;
        }

        if(currentDiff <= 0.0 && prevDiff >= 0.0){
            float aDist = prevDistance;
            float bDist = currentDistance;
            vec2 lastUV = currentUV;
            float lastDepth = currentDepth;

            for(int j = 0; j < MAX_BISECT_STEPS; ++j){
                float midDist = 0.5 * (aDist + bDist);
                vec3 midPoint = position + dir * midDist;
                vec4 midClip = uVPLight * vec4(midPoint, 1.0);
                vec3 midNDC = midClip.xyz / midClip.w;
                vec2 midUV = midNDC.xy * 0.5 + 0.5;

                if(midUV.x < 0.0 || midUV.x > 1.0 || midUV.y < 0.0 || midUV.y > 1.0)
                    break;

                float midRayDepth = midNDC.z * 0.5 + 0.5;
                float midDepth = texture(uShadowMap, midUV).r;
                float midDiff = midDepth - midRayDepth;

                lastUV = midUV;
                lastDepth = midDepth;

                if(abs(midDiff) < DEPTH_EPS){
                    vec3 floorPoint = reconstructFromShadow(midUV, midDepth);
                    return floorPoint - dir * uFloorOffset;
                }

                if(midDiff > 0.0){
                    aDist = midDist;
                } else {
                    bDist = midDist;
                }
            }

            vec3 floorPoint = reconstructFromShadow(lastUV, lastDepth);
            return floorPoint - dir * uFloorOffset;
        }

        prevDistance = currentDistance;
        prevUV = currentUV;
        prevDepth = currentDepth;
        prevDiff = currentDiff;
    }

    if(prevDiff <= 0.0){
        vec3 floorPoint = reconstructFromShadow(prevUV, prevDepth);
        return floorPoint - dir * uFloorOffset;
    }

    return position;
}

void main()
{
    vec2 uv = v_gridPos[0] * 0.5 + 0.5;
    vec4 worldPos = texture(uWaterMask, uv);
    if (worldPos.a < 0.5) return;

    vec3 Ps = worldPos.rgb;

    vec3 Ns = getWaterNormal(uv, Ps);

    vec3 Ri = normalize(Ps - uLightPos);

    vec3 Rt = refract(Ri, Ns, ETA);

    if (length(Rt) == 0.0)
        return;

    vec3 Pi = findFloor(Ps, Rt);

    if(distance(Ps, Pi) < 0.01)
        return;

    float distInWater = distance(Ps, Pi);
    float distToCam = distance(uCamPos, Pi);
    distToCam = max(distToCam, 1e-4);

    float sfinal = uA + uB / distToCam;

    vIntensity = uIntensity * (1.0 - exp(-uAttenuation * distInWater)); // pas exactement comme le papier mais ca rend mieux
    gl_Position = uVPCamera * vec4(Pi, 1.0);
    vClipPos = gl_Position;
    gl_PointSize = sfinal;
    EmitVertex();
    EndPrimitive();

}


