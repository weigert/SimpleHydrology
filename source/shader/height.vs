#version 430 core

layout(location = 0) in vec3 in_Position;

uniform mat4 vp;
out vec3 ex_Position;

void main(void) {
  ex_Position = in_Position;
	gl_Position = vp * vec4(in_Position, 1.0f);
}
