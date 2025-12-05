in vec2 texCoords;

uniform float uNear;
uniform float uFar;

uniform int uIsUnderwater;
uniform vec3 uFogColor;
uniform float uFogDensity;

uniform sampler2D uTexture;

out vec4 fragColor;

void main(){
	vec4 color = texture(uTexture, texCoords);

	if(uIsUnderwater == 1){
		float fogFactor = clamp(1.0 - exp(-uFogDensity), 0.0, 1.0);
		color.rgb = mix(color.rgb, uFogColor, fogFactor);
	}

	fragColor = vec4(color.rgb, 1.0);
}