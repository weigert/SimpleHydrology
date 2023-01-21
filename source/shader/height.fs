#version 430 core

in vec3 ex_Position;
out vec4 fragColor;

void main(void) {

  float heightindex = floor(ex_Position.y/2);
  float shade = 2*heightindex / 80.0;

  fragColor = vec4(vec3(shade),1);
}
