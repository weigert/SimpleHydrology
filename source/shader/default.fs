#version 430 core

flat in vec4 ex_Color;
out vec4 fragColor;

void main(void) {
  fragColor = ex_Color;
}
