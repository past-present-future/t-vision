#version 430 core
out vec4 FragColor;

in vec2 TexCoord;

layout (location = 0) uniform sampler2D textureY;
layout (location = 1) uniform sampler2D textureU;
layout (location = 2) uniform sampler2D textureV;
layout (location = 3) uniform sampler2D freeze_texture;

void main()
{
	mediump float r,g,b,y,u,v;
	vec4 tex_diff;
	y=texture(textureY, TexCoord).r;
	u=texture(textureU, TexCoord).r;
	v=texture(textureV, TexCoord).r;
	tex_diff=texture(freeze_texture, TexCoord);
	y=1.1643*(y-0.0625);
	u=u-0.5;
	v=v-0.5;

	r=y+1.5958*v;
	g=y-0.39173*u-0.81290*v;
	b=y+2.017*u;

	FragColor = vec4(r - tex_diff.r, g - tex_diff.g, b - tex_diff.b, 1.0);

}
