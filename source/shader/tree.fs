#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColor;

in vec4 ex_Position;
in vec3 ex_Normal;

uniform vec3 color;

void main(){

  gColor = vec4(color, 1);
  gPosition = ex_Position.xyz;
  gNormal = normalize(ex_Normal);

}
