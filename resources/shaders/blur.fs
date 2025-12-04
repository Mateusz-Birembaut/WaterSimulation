in vec2 fUV;

out vec4 fragColor;

uniform sampler2D uTexture;
uniform vec2 uTexelSize;
uniform vec2 uDirection;
uniform float uRadius;

const int KERNEL_TAPS = 8;

float gaussianWeight(float x, float sigma) {
    float s = max(sigma, 1e-4);
    return exp(-(x * x) / (2.0 * s * s));
}

void main() {
    vec4 accum = texture(uTexture, fUV);
    float weightSum = 1.0;

    vec2 dir = normalize(uDirection);
    
    if(length(dir) < 1e-4)
        dir = vec2(1.0, 0.0);

    float sigma = max(uRadius, 0.5);

    for(int i = 1; i <= KERNEL_TAPS; ++i) {
        float offset = float(i);
        float weight = gaussianWeight(offset, sigma);

        vec2 sampleOffset = dir * (offset * uTexelSize);

        accum += texture(uTexture, fUV + sampleOffset) * weight;
        accum += texture(uTexture, fUV - sampleOffset) * weight;
        weightSum += 2.0 * weight;
    }

    fragColor = accum / weightSum;
}