#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColor;

in vec3 ex_Position;
in vec2 ex_Tex;

uniform sampler2D spriteTexture;

void main(){

  gColor = texture(spriteTexture, ex_Tex);
  if(gColor.a == 0){
    discard;
    return;
  }
  gPosition = ex_Position;
  gNormal = vec3(0,0,1);

}
