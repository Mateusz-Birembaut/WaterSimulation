in vec2 fUV;

out vec4 fragColor;

uniform sampler2D uTexture;
uniform vec2 uTexelSize;
uniform float uRotation;
uniform float uRadius;

const int POISSON_COUNT = 12;
const vec2 POISSON_DISK[POISSON_COUNT] = vec2[](
    vec2(-0.326212, -0.40581),
    vec2(-0.840144, -0.07358),
    vec2(-0.695914,  0.457137),
    vec2(-0.203345,  0.620716),
    vec2( 0.96234,  -0.194983),
    vec2( 0.473434, -0.480026),
    vec2( 0.519456,  0.767022),
    vec2( 0.185461, -0.893124),
    vec2( 0.507431,  0.064425),
    vec2( 0.89642,   0.412458),
    vec2(-0.32194,  -0.932615),
    vec2(-0.791559, -0.59771)
);

float gaussianFalloff(float r, float sigma) {
    float s = max(sigma, 1e-4);
    return exp(-(r * r) / (2.0 * s * s));
}

void main() {
    float sigma = max(uRadius * 0.5, 0.1);
    float effectiveRadius = max(uRadius, 0.1);

    float s = sin(uRotation);
    float c = cos(uRotation);
    mat2 rotationMatrix = mat2(c, -s, s, c);

    vec4 accum = texture(uTexture, fUV);
    float weightSum = 1.0;

    for(int i = 0; i < POISSON_COUNT; ++i) {
        vec2 rotatedOffset = rotationMatrix * (POISSON_DISK[i] * effectiveRadius);
        vec2 sampleUV = fUV + rotatedOffset * uTexelSize;

        float weight = gaussianFalloff(length(rotatedOffset), sigma);
        accum += texture(uTexture, sampleUV) * weight;
        weightSum += weight;
    }

    fragColor = accum / weightSum;
}