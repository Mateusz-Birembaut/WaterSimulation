in vec4 position; 
in vec3 normal;
in vec2 uv;

uniform mat4 uMVP;
uniform mat4 uLightVP;
uniform sampler2D uHeightMap; 

out vec2 frag_UV;
out vec3 frag_Normal;
out vec4 frag_posLightSpace;

void main() {
    frag_UV = uv;
    frag_Normal = normal;

    vec4 height = texture(uHeightMap, uv);

    vec4 finalPosition = position;
    finalPosition.y = finalPosition.y + (height.r);

    frag_posLightSpace = uLightVP * finalPosition;

    gl_Position = uMVP * finalPosition;
}