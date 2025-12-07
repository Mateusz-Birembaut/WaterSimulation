in vec2 frag_UV;
in vec3 frag_Normal;


uniform sampler2D uAlbedoTexture;
uniform sampler2D uHeightMap; 

out vec4 FragColor;

vec3 shallow_color = vec3(144.0 / 255.0, 213.0 / 255.0, 1.0);
vec3 deep_color = vec3(30.0 / 255.0, 80.0 / 255.0, 180.0 / 255.0);

const float shallow_alpha = 0.35;
const float deep_alpha = 0.75;

void main() {
    vec4 state =  texture(uAlbedoTexture, frag_UV);
    float depth = state.r;
    if(depth <= 1e-3) discard;
    vec3 water_color = mix(shallow_color, deep_color, depth);

    float opacity = clamp(mix(shallow_alpha, deep_alpha, depth), 0.1, 0.85);
    FragColor = vec4(water_color, opacity);

    //FragColor = vec4(0.0, state.g * 10.0, state.b * 10.0, 1.0);
}