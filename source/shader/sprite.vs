#version 430 core

layout(location = 0) in vec3 in_Quad;
layout(location = 1) in vec2 in_Tex;
layout(location = 2) in mat4 in_Model;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 om;

out vec3 ex_Position;
out vec2 ex_Tex;

void main(void) {

	const vec4 v_Position = view * in_Model * om * vec4(in_Quad, 1.0);
	ex_Position = v_Position.xyz;
	gl_Position = proj*v_Position;

	ex_Tex = in_Tex;

}
