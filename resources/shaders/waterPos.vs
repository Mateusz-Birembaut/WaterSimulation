in vec3 position;
in vec2 uv;

uniform mat4 uModel;
uniform mat4 uLightVP;
uniform sampler2D uHeightMap;

out vec3 vWorldPos;
out float vWaterHeight; 

void main() {
    vec3 localPos = position;
    
    float h = texture(uHeightMap, uv).r;
    
    localPos.y += h;
    vWaterHeight = h; 

    vec4 worldPos4 = uModel * vec4(localPos, 1.0);
    vWorldPos = worldPos4.xyz;
    gl_Position = uLightVP * worldPos4;

}