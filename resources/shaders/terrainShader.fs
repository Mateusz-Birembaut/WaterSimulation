in vec2 frag_UV;
in vec3 frag_Normal;


uniform sampler2D uAlbedoTexture;
uniform sampler2D uHeightMap; 

out vec4 FragColor;

vec3 water_color = vec3(144.0/255.0,213.0/255.0,1.0);

void main() {

    FragColor = texture(uAlbedoTexture, frag_UV);
}