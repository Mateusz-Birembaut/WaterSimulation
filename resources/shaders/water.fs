in vec2 frag_UV;
in vec3 frag_Normal;


uniform sampler2D uAlbedoTexture;
uniform sampler2D uHeightMap; 

out vec4 FragColor;

vec3 shallow_color = vec3(144.0 / 255.0, 213.0 / 255.0, 1.0);
vec3 deep_color = vec3(0.0, 0.0, 0.5);

void main() {
    float depth = texture(uAlbedoTexture, frag_UV).r;
    if(depth <= 1e-3) discard;
    vec3 water_color = mix(shallow_color, deep_color, depth);

    FragColor = vec4(water_color, 1.0);
}