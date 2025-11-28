layout (points) in;
layout (points, max_vertices = 1) out;

in vec2 v_gridPos[]; 

uniform sampler2D uWaterMask; // voir pour réduire la taille de la texture si besoin
uniform sampler2D uShadowMap;


uniform mat4 uVPLight;   
uniform mat4 uVPCamera;   
uniform vec3 uCamPos;     
uniform vec3 uLightPos;   


const float IOR_AIR = 1.0;
const float IOR_WATER = 1.33;
const float ETA = IOR_AIR / IOR_WATER;


out float vIntensity; 
out vec3 vNs;


vec3 getWaterNormal(vec2 uv, vec3 currentPos) {
    //vec2 texSize = textureSize(uWaterMask, 0);
    //vec2 offset = vec2(1.0, 1.0) / texSize;

    vec3 posRight = texture(uWaterMask, uv + vec2(0.01, 0.0)).rgb;
    vec3 posUp = texture(uWaterMask, uv + vec2(0.0, 0.01)).rgb;

    vec3 v1 = posRight - currentPos;
    vec3 v2 = posUp - currentPos;

    return normalize(cross(v2, v1));
}



void main() {
    vec2 uv = v_gridPos[0] * 0.5 + 0.5;
    vec4 worldPos = texture(uWaterMask, uv);
    
    if (worldPos.a < 0.5) return;
    
    vec3 Ps = worldPos.rgb; // ok


    vec3 Ns = getWaterNormal(uv, Ps);
    vec3 incident = normalize(Ps - uLightPos);
    vec3 refractDir = refract(incident, Ns, ETA);

    gl_Position = uVPCamera * vec4(Ps, 1.0);
    gl_PointSize = 5.0;
    vNs = incident; 
    vIntensity = 1.0; 
    EmitVertex();
    EndPrimitive();
    return;
    
    //if (length(refractDir) == 0.0) return;

}