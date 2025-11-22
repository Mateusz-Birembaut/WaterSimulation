#version 330

out vec4 fragColor;

in vec3 vPos;
in vec3 vNormal;
in vec2 vUV;

uniform vec3 cameraPos;

const vec3 light_pos = vec3(0.0, 5.0, 0.0);
const vec3 light_color = vec3(1.0, 1.0, 1.0);
const vec3 water_color = vec3(0.788, 0.467, 0.149);
const vec3 foam_color = vec3(0.8, 0.8, 0.8);

const float shininess = 16.0;

float checkers( in vec2 p ){
    vec2 q = floor(p);
    return mod(q.x+q.y,2.);
}

float fresnel(vec3 viewDir, vec3 normal) {
    float f = 1.0 - max(dot(viewDir, normal), 0.0);
    return pow(f, 3.0) * 0.65;
}

void main() {
    vec3 lightDir = normalize(light_pos - vPos);
    vec3 viewDir = normalize(cameraPos - vPos);
    vec3 reflectDir = reflect(-lightDir, vNormal);

    float diff = max(dot(vNormal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    float fresnelEffect = fresnel(viewDir, vNormal);

    vec3 baseColor = mix(water_color, foam_color, smoothstep(0.05, 1.0, vPos.y));
    //vec3 finalColor = baseColor * diff * light_color + spec * light_color + fresnelEffect * water_color;

    vec3 finalColor = bool(checkers(vPos.xz*1.)) ? water_color : water_color * 0.75 ; 

    fragColor = vec4(finalColor, 1.0);
}
