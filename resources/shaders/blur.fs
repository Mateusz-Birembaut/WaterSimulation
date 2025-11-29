in vec2 fUV;

out vec4 fragColor;

uniform sampler2D uTexture;

const float PI = 3.14159265359;
const int SAMPLES = 32;        
const float BLUR_RADIUS = 4.0; 


float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    ivec2 size = textureSize(uTexture, 0);
    vec2 texelSize = 1.0 / vec2(size); 

    vec4 totalColor = texture(uTexture, fUV);
    float totalWeight = 1.0;

    float noise = rand(fUV);
    float angle = noise * 2.0 * PI;
    
    float s = sin(angle);
    float c = cos(angle);
    mat2 rotationMatrix = mat2(c, -s, s, c);

    for(int i = 1; i < SAMPLES; i++) {
        float scale = float(i) / float(SAMPLES - 1); 
        float currentRadius = BLUR_RADIUS * scale;

        vec2 offset = vec2(currentRadius, 0.0);

        offset = rotationMatrix * offset;

        vec2 offsetUV = offset * texelSize;

        vec4 sample1 = texture(uTexture, fUV + offsetUV);
        vec4 sample2 = texture(uTexture, fUV - offsetUV);

        totalColor += sample1 + sample2;
        totalWeight += 2.0;
    }


    fragColor = totalColor / totalWeight;
}