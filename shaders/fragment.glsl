#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D frameTexture;

void main()
{
    FragColor = texture(frameTexture, TexCoord);
}
