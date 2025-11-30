in vec2 frag_UV;
in vec3 frag_Normal;
in vec4 frag_posLightSpace;

uniform sampler2D uAlbedoTexture;
uniform sampler2D uHeightMap; 
uniform sampler2D uShadowMap;

out vec4 FragColor;

void main() {

    vec3 projCoords = frag_posLightSpace.xyz / frag_posLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float currentDepth = projCoords.z;
    float shadowMapDepth = texture(uShadowMap, projCoords.xy).r;

    float depthBias = 0.005; 
    float shadow = currentDepth - depthBias > shadowMapDepth ? 0.7 : 1.0;

    FragColor = texture(uAlbedoTexture, frag_UV) * shadow;
}