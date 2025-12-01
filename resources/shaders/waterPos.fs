
in vec3 vWorldPos;
in float vWaterHeight;

out vec4 FragColor;


void main() {

	if (vWaterHeight < 0.001) {
        discard; 
    }

    FragColor = vec4(vWorldPos, 1.0);
    
}