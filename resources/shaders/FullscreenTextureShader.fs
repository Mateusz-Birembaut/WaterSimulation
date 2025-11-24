in vec2 texCoords;

uniform float uNear;
uniform float uFar;

uniform sampler2D uTexture;

out vec4 fragColor;

void main(){
	vec4 color = texture(uTexture, texCoords);
	fragColor = vec4(vec3(color), 1.0);
}