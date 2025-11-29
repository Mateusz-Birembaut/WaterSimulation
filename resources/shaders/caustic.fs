in float vIntensity; 
in vec4 vClipPos;

out vec4 FragColor;

uniform sampler2D uCamDepthBuffer;


void main() {
    vec3 ndc = vClipPos.xyz / vClipPos.w;   
    vec2 screenUV = ndc.xy * 0.5 + 0.5;   
    float sceneDepth = texture(uCamDepthBuffer, screenUV).r;

    float bias = 0.01; 
    if (gl_FragCoord.z > sceneDepth) {
        discard;
    }

    vec2 coord = gl_PointCoord * 2.0 - 1.0;

    float distSquared = dot(coord, coord);

    if (distSquared > 1.0) {
        discard;
    }

    float alpha = exp(-distSquared * 4.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0); 

    vec3 finalRGB = lightColor * vIntensity * alpha;

    FragColor = vec4(finalRGB, 1.0);
}