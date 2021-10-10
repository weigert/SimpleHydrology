#version 430 core

in vec2 ex_Tex;

uniform sampler2D imageTexture;

out vec4 fragColor;

void main(){
  fragColor = texture(imageTexture, ex_Tex);
}
