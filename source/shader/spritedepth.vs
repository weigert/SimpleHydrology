#version 430 core

layout(location = 0) in vec3 in_Quad;
layout(location = 1) in vec2 in_Tex;
layout(location = 2) in mat4 in_Model;

uniform mat4 dvp;
uniform mat4 om;

out vec2 ex_Tex;

void main(){

  ex_Tex = in_Tex;

  gl_Position = dvp*in_Model*om*vec4(in_Quad, 1.0);

}
