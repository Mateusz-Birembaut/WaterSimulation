in vec2 frag_UV;
in vec3 frag_Normal;


uniform sampler2D uAlbedoTexture;
uniform sampler2D uHeightMap; 

out vec4 FragColor;

void main() {

    vec4 h1 = texture(uAlbedoTexture, frag_UV);

    FragColor = vec4(h1.x,h1.x,h1.x,1.0);
}