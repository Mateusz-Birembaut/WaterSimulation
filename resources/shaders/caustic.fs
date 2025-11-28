out vec4 fragColor;

in vec3 vNs;

void main() {
    vec3 color = vNs * 0.5 + 0.5; // Remap [-1,1] vers [0,1]
    fragColor = vec4(color, 1.0); 
}