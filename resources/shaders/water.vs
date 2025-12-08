in vec4 position; 
in vec3 normal;
in vec2 uv;

uniform mat4 uMVP;
uniform sampler2D uHeightMap;
uniform sampler2D uAlbedoTexture;

out vec2 frag_UV;
out vec3 frag_Normal;



void main() {
    frag_UV = uv;
    frag_Normal = normal;

    vec4 height = texture(uHeightMap, uv);

    vec4 heightWater = texture(uAlbedoTexture,uv);

    vec4 finalPosition = position;
    float total_height = (height.r) + heightWater.r ;
    finalPosition.y = finalPosition.y + total_height * 4.0;
    

    gl_Position = uMVP * finalPosition;
}