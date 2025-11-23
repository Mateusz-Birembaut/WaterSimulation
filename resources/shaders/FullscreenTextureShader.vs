in vec4 position;
in vec2 textureCoordinates;

out vec2 texCoords;

void main(){
	gl_Position = position;
	texCoords = textureCoordinates;
}



