in vec3 position;

out vec2  v_gridPos;

void main() {
    v_gridPos = position.xz;
    gl_Position = vec4(position.xz, 0.00, 1.0);
}
