layout (points) in;
layout (points, max_vertices = 1) out;

in vec2 v_gridPos[];

uniform sampler2D uWaterMask;
uniform sampler2D uShadowMap;
uniform mat4 uVPLight;
uniform mat4 uVPCamera;

uniform vec3 uCamPos;
uniform vec3 uLightPos;

uniform float uAttenuation;
uniform float uIntensity;
uniform float uA;
uniform float uB;
uniform float uLightFar;

uniform float uTime;


const float IOR_AIR   = 1.0;
const float IOR_WATER = 1.33;
const float ETA       = IOR_AIR / IOR_WATER;

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

void emitLine(vec3 p0, vec3 p1, vec3 color)
{
    //vNs = color;
    gl_Position = uVPCamera * vec4(p0, 1.0);
    EmitVertex();

    //vNs = color;
    gl_Position = uVPCamera * vec4(p1, 1.0);
    EmitVertex();

    EndPrimitive();
}

void emitPoint (vec3 p, vec3 color, float size)
{
    gl_Position = uVPCamera * vec4(p, 1.0);
    //vNs = color;
    gl_PointSize = size;
    EmitVertex();
    EndPrimitive();
}


float getWaveHeight(vec2 p, float time) {
    float h = 0.0;
    
    // Vague 1 : La houle principale (Lente, grande amplitude)
    // Direction diagonale (p.x + p.y)
    h += sin(p.x * 0.5 + p.y * 0.5 + time * 1.0) * 0.4;

    // Vague 2 : Vague croisée (Plus rapide, amplitude moyenne)
    // Direction opposée et angle différent (p.x * 1.0 - p.y * 0.8)
    h += sin(p.x * 1.0 - p.y * 0.8 + time * 1.4) * 0.2;

    // Vague 3 : Petits clapotis (Rapide, faible amplitude)
    // Casse la symétrie restante
    h += sin(-p.x * 1.5 + p.y * 0.3 + time * 2.5) * 0.1;

    return h;
}

// Correction 2 : Recalculer h_center localement pour être cohérent
vec3 getProceduralNormal(vec2 p, float time) {
    vec2 e = vec2(0.01, 0.0);
    
    // On recalcule les 3 hauteurs avec la MÊME fonction
    // p est déjà Ps.xz
    float h_center = getWaveHeight(p, time);
    float h_right  = getWaveHeight(p + e.xy, time);
    float h_up     = getWaveHeight(p + e.yx, time);
    
    // Maintenant la différence est juste (ex: 0.11 - 0.10 = 0.01)
    vec3 v1 = vec3(e.x, h_right - h_center, 0.0);
    vec3 v2 = vec3(0.0, h_up - h_center, e.x);
    
    return normalize(cross(v2, v1)); 
}


vec3 findFloor(vec3 position, vec3 direction){
    const int MAX_STEPS = 32;
    const int MAX_BISECT_STEPS = 5; 
    const float DEPTH_EPS = 0.0001;
    float maxDistance = uLightFar;
    float stepSize = maxDistance / float(MAX_STEPS);

    vec3 prevPoint = position;
    float prevDiff = 0.0;

    for(int i = 0; i < MAX_STEPS; ++i){
        vec3 currentPoint = position + direction * (stepSize * float(i));
        vec4 lightClip = uVPLight * vec4(currentPoint, 1.0);
        vec3 lightNDC = lightClip.xyz / lightClip.w;
        vec2 shadowUV = lightNDC.xy * 0.5 + 0.5;

        if(shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0)
            return prevPoint;

        float currentRayDepth = lightNDC.z * 0.5 + 0.5;
        float currentUVDepth = texture(uShadowMap, shadowUV).r;
        float diff = currentUVDepth - currentRayDepth;

        if(i == 0){
            prevDiff = diff;
            prevPoint = currentPoint;
            continue;
        }

        if(abs(diff) < DEPTH_EPS)
            return currentPoint;

        // Once we detect that the ray went behind the stored depth we bisect to stay on top of the floor
        if(diff <= 0.0 && prevDiff >= 0.0){
            vec3 a = prevPoint;
            vec3 b = currentPoint;
            for(int j = 0; j < MAX_BISECT_STEPS; ++j){
                vec3 midPoint = mix(a, b, 0.5);
                vec4 midClip = uVPLight * vec4(midPoint, 1.0);
                vec3 midNDC = midClip.xyz / midClip.w;
                vec2 midUV = midNDC.xy * 0.5 + 0.5;
                if(midUV.x < 0.0 || midUV.x > 1.0 || midUV.y < 0.0 || midUV.y > 1.0)
                    break;

                float midDepth = midNDC.z * 0.5 + 0.5;
                float storedDepth = texture(uShadowMap, midUV).r;
                float midDiff = storedDepth - midDepth;

                if(abs(midDiff) < DEPTH_EPS)
                    return midPoint;

                if(midDiff > 0.0)
                    a = midPoint;
                else
                    b = midPoint;
            }
            return a;
        }

        prevDiff = diff;
        prevPoint = currentPoint;
    }

    return prevPoint;
}


void main()
{
    vec2 uv = v_gridPos[0] * 0.5 + 0.5;
    vec4 worldPos = texture(uWaterMask, uv);
    if (worldPos.a < 0.5) return;

    vec3 Ps = worldPos.rgb;

    //float waveH = getWaveHeight(Ps.xz, uTime);
    //Ps.y += waveH; 
    //vec3 Ns = getProceduralNormal(Ps.xz, uTime);
    
    vec3 Ns = getWaterNormal(uv, Ps);

    vec3 Ri = normalize(uLightPos - Ps);
    vec3 Rt = refract(Ri, Ns, ETA);

    if (length(Rt) == 0.0)
        return;

    vec3 Pi = findFloor(Ps, Rt);

    float distInWater = distance(Ps, Pi);
    float distToCam = length((uVPCamera * vec4(Ps,1.0)).xyz);
    
    float sfinal = uA + uB / distToCam;

    /*
    vIntensity = uIntensity * exp(-uAttenuation * distInWater);
    gl_Position = uVPCamera * vec4(Ps, 1.0);
    vClipPos = gl_Position;
    gl_PointSize = sfinal;
    EmitVertex();
    EndPrimitive();
    */

    vIntensity = uIntensity * exp(-uAttenuation * distInWater);
    gl_Position = uVPCamera * vec4(Pi, 1.0);
    vClipPos = gl_Position;
    gl_PointSize = sfinal;
    EmitVertex();
    EndPrimitive();

}


