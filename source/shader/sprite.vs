#version 430 core

layout(location = 0) in vec3 in_Quad;
layout(location = 1) in vec2 in_Tex;
layout(location = 2) in mat4 in_Model;

uniform mat4 vp;
uniform mat4 om;

out vec2 ex_Tex;

void main(void) {

	//Pass Texture Coordinates
	ex_Tex = vec2(1.0f-in_Tex.x, in_Tex.y);

	//Actual Position in Space
	gl_Position = vp * in_Model * om * vec4(in_Quad, 1.0f);

}
