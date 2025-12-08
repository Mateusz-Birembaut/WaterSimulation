layout (points) in;
layout (line_strip, max_vertices = 2) out; // TODO line width modifialb => faire des triangles strip

in vec2 v_gridPos[];

uniform sampler2D uWaterMask;
uniform sampler2D uShadowMap;
uniform mat4 uVPLight;
uniform mat4 uVPCamera;

uniform vec3 uCamPos;
uniform vec3 uLightPos;

uniform float uG;
uniform float uLightFar;
uniform float uRaywidth;

uniform float uTime;

const float IOR_AIR   = 1.0;
const float IOR_WATER = 1.33;
const float ETA       = IOR_AIR / IOR_WATER;

out float gDistFromSurface;
out float gDistFromViewer; 
out vec3 gViewDir;          
out vec3 gLightDir;         
out vec4 gClipPos;


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
    vec3 p = position;
    int MAX_ITER = 15;

    for(int i = 0; i < MAX_ITER; i++){

        vec4 lightClip = uVPLight * vec4(p, 1.0);
        vec3 lightNDC = lightClip.xyz / lightClip.w; // -1 1
        vec2 shadowUV = lightNDC.xy * 0.5 + 0.5;

        if(shadowUV.x < 0 || shadowUV.x > 1 || shadowUV.y < 0 || shadowUV.y > 1) break;

        float currentRayDepth = lightNDC.z * 0.5 + 0.5; // 0 1
        float currentUVDepth = texture(uShadowMap, shadowUV).r; // 0 1 
        float diff = currentUVDepth - currentRayDepth;

        if (abs(diff) < 0.0001) break;

        float displacementDist = diff * uLightFar;

        p += direction * displacementDist;

        // TODO ajouter le fait de partir dans l'autre sens pour des height maps plus complexe mais pour l'instant ok

    }
    return p;
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

	gl_Position = uVPCamera * vec4(Ps, 1.0);
	gClipPos = gl_Position;
    gDistFromSurface = 0.0;                 
    gDistFromViewer = distance(Ps, uCamPos);
    gViewDir = normalize(uCamPos - Ps);
    gLightDir = Ri;                
    
    EmitVertex();


    gl_Position = uVPCamera * vec4(Pi, 1.0);
    gClipPos = gl_Position;
    gDistFromSurface = distance(Ps, Pi);     
    gDistFromViewer  = distance(Pi, uCamPos);
    gViewDir = normalize(uCamPos - Pi);
    gLightDir = Ri;                   
    
    EmitVertex();


    EndPrimitive();


}


