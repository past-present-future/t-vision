#version 430 core
out vec4 FragColor;

in vec2 TexCoord;

layout (location = 0) uniform sampler2D textureY;
layout (location = 1) uniform sampler2D textureU;
layout (location = 2) uniform sampler2D textureV;
layout (location = 3) uniform sampler2D still_textureY;
layout (location = 4) uniform sampler2D still_textureU;
layout (location = 5) uniform sampler2D still_textureV;

void main()
{
  mediump float r, g, b, y, u, v, still_y, still_u, still_v,  still_r,still_g,still_b;
  vec4 tex_diff;
  y=texture(textureY, TexCoord).r;
  u=texture(textureU, TexCoord).r;
  v=texture(textureV, TexCoord).r;

  y=1.1643*(y-0.0625);
  u=u-0.5;
  v=v-0.5;

  r=y+1.596*v;
  g=y-0.391*u-0.8130*v;
  b=y+2.018*u;

  still_y = texture(still_textureY, TexCoord).r;
  still_u = texture(still_textureU, TexCoord).r;
  still_v = texture(still_textureV, TexCoord).r;

  still_y=1.1643*(still_y-0.0625);
  still_u=still_u-0.5;
  still_v=still_v-0.5;

  still_r=still_y+1.596*still_v;
  still_g=still_y-0.391*still_u-0.8130*still_v;
  still_b=still_y+2.018*u;

  tex_diff = vec4(clamp(still_r - r, 0.0, 1.0), clamp(still_g - g, 0.0, 1.0), clamp(still_b - b, 0.0, 1.0), 1.0);

  FragColor = vec4(0.0 + tex_diff.r, 0.0 + tex_diff.g, 0.0 + tex_diff.b, 1.0);

}
