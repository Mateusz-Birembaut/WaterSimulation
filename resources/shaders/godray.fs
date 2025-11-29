
in float gDistFromSurface; 
in float gDistFromViewer; 
in vec3 gViewDir;       
in vec3 gLightDir;  
in vec4 gClipPos;  

out vec4 FragColor;


uniform sampler2D uCamDepthBuffer;

uniform float uG;          
uniform float uIntensity;   
uniform float uFogDensity;


float computeMie(float dotViewLight, float g) {
    float g2 = g * g;
    float result = 1.0 - g2;
    float denom = 1.0 + g2 - 2.0 * g * dotViewLight; 
    return result / (4.0 * 3.14159 * pow(denom, 1.5));
}

void main() {

    vec3 ndc = gClipPos.xyz / gClipPos.w;   
    vec2 screenUV = ndc.xy * 0.5 + 0.5;   
    
    float sceneDepth = texture(uCamDepthBuffer, screenUV).r;

    if (gl_FragCoord.z > sceneDepth) {
        discard;
    }
    
    float dotVL = dot(normalize(gViewDir), normalize(gLightDir));
    float mie = computeMie(dotVL, uG);

    float attenuation = exp(-uFogDensity * (gDistFromSurface + gDistFromViewer));


    vec3 rayColor = vec3(0.8, 0.9, 1.0);
    
    float finalAlpha = uIntensity * mie * attenuation;
    
    FragColor = vec4(rayColor * finalAlpha, 1.0);
}