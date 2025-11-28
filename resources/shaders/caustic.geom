layout (points) in;
layout (points, max_vertices = 1) out;

in vec2 v_gridPos[]; 

uniform sampler2D uWaterMask; // voir pour réduire la taille de la texture si besoin
uniform sampler2D uShadowMap;


uniform mat4 uVPLight;   
uniform mat4 uVPCamera;   
uniform vec3 uCamPos;     
uniform vec3 uLightPos;   

//TODO : ajouter uniform cam near et far

const float IOR_AIR = 1.0;
const float IOR_WATER = 1.33;
const float ETA = IOR_AIR / IOR_WATER;

const float POINT_SIZE_MIN = 5.0;
const float POINT_SIZE_MAX = 20.0; // a changer en fonction de la taille de grid ect.
const float POINT_SCALE_FACTOR = 150.0;

out float vIntensity; 
out vec3 vNs;


vec3 getWaterNormal(vec2 uv, vec3 posCenter) {
    vec2 texSize = vec2(textureSize(uWaterMask, 0));
    vec2 off = vec2(0.01,0.01); 

    vec3 tX;
    vec4 rawRight = texture(uWaterMask, uv + vec2(off.x, 0.0));
    vec4 rawLeft  = texture(uWaterMask, uv - vec2(off.x, 0.0));

    if (rawRight.a > 0.9 && distance(rawRight.rgb, posCenter) > 0.001) {
        tX = rawRight.rgb - posCenter;
    } 
    else if (rawLeft.a > 0.9 && distance(rawLeft.rgb, posCenter) > 0.001) {
        tX = posCenter - rawLeft.rgb; 
    }
    else {
        tX = vec3(1.0, 0.0, 0.0);
    }

    vec3 tZ;
    vec4 rawUp   = texture(uWaterMask, uv + vec2(0.0, off.y));
    vec4 rawDown = texture(uWaterMask, uv - vec2(0.0, off.y));

    if (rawUp.a > 0.9 && distance(rawUp.rgb, posCenter) > 0.001) {
        tZ = rawUp.rgb - posCenter;
    } 
    else if (rawDown.a > 0.9 && distance(rawDown.rgb, posCenter) > 0.001) {
        tZ = posCenter - rawDown.rgb;
    }
    else {
        tZ = vec3(0.0, 0.0, 1.0); 
    }

    return normalize(cross(tZ, tX)); 
}


void main() {
    vec2 uv = v_gridPos[0] * 0.5 + 0.5;
    vec4 worldPos = texture(uWaterMask, uv);
    
    if (worldPos.a < 0.5) return;
    
    vec3 Ps = worldPos.rgb;
    vec3 Ns = getWaterNormal(uv, Ps);
    vec3 Ri = normalize(uLightPos - Ps);
    vec3 Rt = refract(Ri, Ns, ETA);

    if (length(Rt) == 0.0) return;

    gl_Position = uVPCamera * vec4(Ps, 1.0);
    vNs = Ns;
    gl_PointSize = 5.0;
    EmitVertex();
    EndPrimitive();
    return;

    

}