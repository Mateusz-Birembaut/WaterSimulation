in vec2 frag_UV;
in vec3 frag_Normal;


uniform vec4 uColor;         
uniform bool uHasAlbedo;      
uniform sampler2D uAlbedoTexture; 

out vec4 FragColor;

void main() {
    vec4 finalColor = uColor;

    if (uHasAlbedo) {
        vec4 texColor = texture(uAlbedoTexture, frag_UV);
        finalColor = texColor;
    }

    FragColor = finalColor;
}