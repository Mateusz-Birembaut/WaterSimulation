in float vIntensity; 

out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;

    float distSquared = dot(coord, coord);

    if (distSquared > 1.0) {
        discard;
    }

    float alpha = exp(-distSquared * 4.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0); 

    vec3 finalRGB = lightColor * vIntensity * alpha;

    FragColor = vec4(finalRGB, 1.0);
}