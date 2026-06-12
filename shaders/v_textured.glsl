#version 330 core

uniform sampler2D textureMap;

in vec2 i_tc;

out vec4 pixelColor;

void main()
{
    pixelColor = texture(textureMap, i_tc);
}