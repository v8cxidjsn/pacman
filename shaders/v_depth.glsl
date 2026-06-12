#version 330

uniform mat4 lightSpaceMatrix;
uniform mat4 M;

layout (location = 0) in vec4 vertex;

void main(void) {
    gl_Position = lightSpaceMatrix * M * vertex;
}
