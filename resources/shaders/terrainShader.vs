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


vec3 getNormal(vec2 uv)
{
    ivec2 size = textureSize(uHeightMap, 0);
    vec2 texel = 1.0 / vec2(size);

    vec2 uvL = clamp(uv - vec2(texel.x, 0.0), 0.0, 1.0);
    vec2 uvR = clamp(uv + vec2(texel.x, 0.0), 0.0, 1.0);
    vec2 uvD = clamp(uv - vec2(0.0, texel.y), 0.0, 1.0);
    vec2 uvU = clamp(uv + vec2(0.0, texel.y), 0.0, 1.0);

    float hL = texture(uHeightMap, uvL).r * 1.5;
    float hR = texture(uHeightMap, uvR).r * 1.5;;
    float hD = texture(uHeightMap, uvD).r * 1.5;;
    float hU = texture(uHeightMap, uvU).r * 1.5;;

    float dx = (hR - hL) * 1.5;
    float dz = (hU - hD) * 1.5;

    vec3 n = normalize(vec3(-dx, 2.0, -dz));
    if(n.y < 0.0) n = -n;
    return n;
}

void main()
{
    frag_UV = uv;

    vec3 localNormal = getNormal(uv);

    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    
    frag_Normal = normalize(normalMatrix * localNormal);

    float h = texture(uHeightMap, uv).r;
    vec4 displaced = vec4(position, 1.0);
    displaced.y += h * 1.5;

    vec4 world = uModel * displaced;
    frag_WorldPos = world.xyz;

    frag_posLightSpace = uLightVP * world;

    gl_Position = uMVP * displaced;
}
