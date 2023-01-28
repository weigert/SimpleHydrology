#version 430 core

layout(location = 0) in vec3 in_Position;
uniform mat4 v;
uniform mat4 p;
out vec4 ex_Position;

void main(){

  ex_Position = p * v *vec4(in_Position, 1.0f);
	gl_Position =  ex_Position;

}
