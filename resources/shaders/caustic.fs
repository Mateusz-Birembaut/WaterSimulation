in float vIntensity;
in vec3 vNs;

out vec4 FragColor;


void main() {
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0; 
    
    float dist = dot(circCoord, circCoord); 
    
    if (dist > 1.0) discard;

    //vec3 color = vec3(1.0, 1.0, 1.0) * vIntensity;
    vec3 color = normalize(vNs) * 0.5 + 0.5;

    FragColor = vec4(color, 1.0);
}