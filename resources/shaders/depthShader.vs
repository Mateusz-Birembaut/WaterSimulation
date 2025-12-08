in vec3 position;
in vec2 uv;

uniform mat4 mvp;
uniform sampler2D uHeightMap;
uniform bool hasHeightMap;

void main() {
    float height = 0.0;
    if (hasHeightMap) {
        height = texture(uHeightMap, uv).r;
    }
    vec3 finalPosition = position + vec3(0.0, height, 0.0);
    gl_Position = mvp * vec4(finalPosition, 1.0);
}



