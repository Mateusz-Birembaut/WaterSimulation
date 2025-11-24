// magnum already write #version itself so you cant write it here,
// #version 430
out vec4 fragColor;

in vec3 vPos;
in vec3 vNormal;
in vec2 vUV;

layout(binding = 0) uniform sampler2D debugTexture;
layout(binding = 1) uniform sampler2D debugTexture2;

uniform float scaleFactor = 1.0 / 5.0;

void main() {
    vec2 uv = vUV * 2.0;

    if (uv.x < 1.0 && uv.y < 1.0) {
        float c = abs(texture(debugTexture, uv).r) * scaleFactor;
        fragColor = vec4(c, c, c, 1.0);
    } else if (uv.x >= 1.0 && uv.y < 1.0) {
        fragColor =
            vec4(0.0, abs(texture(debugTexture, vec2(uv.x - 1.0, uv.y)).g), 0.0,
                 1.0);
    } else if (uv.x < 1.0 && uv.y >= 1.0) {
        fragColor =
            vec4(0.0, 0.0, abs(texture(debugTexture, vec2(uv.x, uv.y - 1.0)).b),
                 1.0);
    } else {
        fragColor =
            vec4(abs(texture(debugTexture2, vec2(uv.x - 1.0, uv.y - 1.0)).r));
    }
}
