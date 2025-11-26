in vec2 frag_uv;

uniform sampler2D depthTexture;
uniform float near;
uniform float far;
uniform bool isOrthographic; 
uniform float uLinearizeRange;

out vec4 fragColor;

// ...existing code...
void main() {
    float depth = texture(depthTexture, frag_uv).r;

    // debug quick raw depth (optional)
    // fragColor = vec4(vec3(depth), 1.0);

    // Linearize
    float viewSpaceZ = 0.0;
    if (isOrthographic) {
        viewSpaceZ = near + depth * (far - near);
    } else {
        float z_ndc = depth * 2.0 - 1.0; 
        viewSpaceZ = (2.0 * near * far) / (far + near - z_ndc * (far - near));
    }

    // Map to visually useful range
    float visualRange = uLinearizeRange; // régler pour ta scène
    float displayVal = viewSpaceZ / visualRange;
    displayVal = clamp(displayVal, 0.0, 1.0);

    // Optionnel: vraiment mettre en évidence
    // fragColor = vec4(vec3(1.0 - depth), 1.0); // raw inverted
    fragColor = vec4(vec3(displayVal), 1.0);
}