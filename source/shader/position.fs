#version 430 core

in vec4 ex_Position;
uniform mat4 v;
out vec4 fragColor;

void main(void) {

  fragColor = vec4(0.5 + 0.5*ex_Position.xyz, 1);

}
