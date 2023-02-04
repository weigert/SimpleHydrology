#version 430 core

layout(location = 0) in vec4 in_Pos;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in mat4 in_Model;

uniform mat4 dvp;

void main(){

  gl_Position = dvp*in_Model*in_Pos;

}
