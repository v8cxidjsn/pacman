#version 330

//Zmienne jednorodne
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

//Atrybuty
layout (location=0) in vec4 vertex; //wspolrzedne wierzcholka w przestrzeni modelu
layout (location=1) in vec4 normal; //wektor normalny w wierzcholku
layout (location=2) in vec2 texCoord; //wspolrzedne tekstury

out vec2 i_tc;
out vec3 i_worldPos;
out vec3 i_normal;

void main(void) {
    vec4 worldPos = M * vertex;
    mat3 normalMatrix = transpose(inverse(mat3(M)));

    gl_Position = P * V * worldPos;
    i_tc = texCoord;
    i_worldPos = worldPos.xyz;
    i_normal = normalize(normalMatrix * normal.xyz);
}
