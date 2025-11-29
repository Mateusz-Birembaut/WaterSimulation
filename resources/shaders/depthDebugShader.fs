in vec2 frag_uv;

uniform sampler2D depthTexture;
uniform float near;
uniform float far;
uniform bool isOrthographic; 
uniform float uLinearizeRange;

out vec4 fragColor;


void main() {
    float depth = texture(depthTexture, frag_uv).r;


    float viewSpaceZ = 0.0;
    if (isOrthographic) {
        viewSpaceZ = near + depth * (far - near);
    } else {
        float z_ndc = depth * 2.0 - 1.0; 
        viewSpaceZ = (2.0 * near * far) / (far + near - z_ndc * (far - near));
    }


    float visualRange = uLinearizeRange; 
    float displayVal = viewSpaceZ / visualRange;
    displayVal = clamp(displayVal, 0.0, 1.0);


    fragColor = vec4(vec3(displayVal), 1.0);
}