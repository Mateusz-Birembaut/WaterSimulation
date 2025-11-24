in vec2 frag_UV;
in vec3 frag_Normal;


uniform sampler2D uAlbedoTexture;
uniform sampler2D uHeightMap; 

out vec4 FragColor;

void main() {

    FragColor = texture(uAlbedoTexture, frag_UV);
}