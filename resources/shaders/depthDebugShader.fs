in vec2 texCoords;

layout(location = 0) uniform float near;
layout(location = 1) uniform float far;

layout(binding = 0) uniform sampler2D depthTexture;

out vec4 fragColor;

void main(){
	float depth = texture(depthTexture, texCoords).r;
	fragColor = vec4(vec3(1-depth), 1.0);
}