in vec3 position; 
in vec3 normal;
in vec2 uv;

uniform mat4 uModel;
uniform mat4 uMVP;
uniform mat4 uLightVP;
uniform sampler2D uHeightMap;

out vec2 frag_UV;
out vec3 frag_Normal;
out vec4 frag_posLightSpace;
out vec3 frag_WorldPos;

void main()
{
    frag_UV = uv;


    mat3 normalMatrix = mat3(uModel);
    frag_Normal = normalize(normalMatrix * normal);


    float h = texture(uHeightMap, uv).r;
    vec4 displaced = vec4(position, 1.0);
    displaced.y += h * 1.5;

    vec4 world = uModel * displaced;
    frag_WorldPos = world.xyz;


    frag_posLightSpace = uLightVP * world;

    gl_Position = uMVP * displaced;
}
