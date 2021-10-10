#version 430 core

in vec2 ex_Tex;
out vec4 fragColor;

uniform sampler2D spriteTexture;

void main(){

  vec4 color = texture(spriteTexture, ex_Tex);

  if(color.a == 0.0) discard;
  else fragColor = color;

}
