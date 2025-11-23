in vec4 position; 
in vec3 normal;
in vec2 uv;

uniform mat4 uMVP;


out vec2 frag_UV;
out vec3 frag_Normal;

void main() {
    frag_UV = uv;
    frag_Normal = normal;

    gl_Position = uMVP * position;
}