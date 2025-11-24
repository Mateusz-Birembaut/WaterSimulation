in vec4 position; 
in vec3 normal;
in vec2 uv;

uniform mat4 uMVP;
uniform sampler2D uHeightMap; 

out vec2 frag_UV;
out vec3 frag_Normal;

void main() {
    frag_UV = uv;
    frag_Normal = normal;

    vec4 height = texture(uHeightMap, uv);

    vec4 finalPosition = position;
    finalPosition.y = finalPosition.y + (height.r) * 2.0;

    gl_Position = uMVP * finalPosition;
}