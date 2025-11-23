in vec2 frag_UV;
in vec3 frag_Normal;


uniform sampler2D uAlbedoTexture; 

out vec4 FragColor;

void main() {

    FragColor = texture(uAlbedoTexture, frag_UV);
}