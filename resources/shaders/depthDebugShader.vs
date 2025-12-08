in vec3 position;
in vec2 uv;

uniform mat4 mvp;

out vec2 frag_uv;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    frag_uv = uv;
}
