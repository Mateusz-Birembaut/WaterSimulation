#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

uniform mat4 MVP;
uniform int dx;
uniform int dy;

out vec3 vPos;
out vec3 vNormal;
out vec2 vUV;

void main()
{
    vPos = aPos;
    vNormal = aNormal;
    vUV = aUV;
    gl_Position = MVP * vec4(aPos, 1.0);

}