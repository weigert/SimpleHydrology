#version 430 core

layout(location = 0) in vec3 in_Position;

uniform mat4 dvp;

void main(void) {
	gl_Position = dvp * vec4(in_Position, 1.0f);
}
