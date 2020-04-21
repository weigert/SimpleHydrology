#version 130
in vec3 in_Quad;
in vec2 in_Tex;
in mat4 in_Model;

uniform mat4 projectionCamera;

out vec2 ex_Tex;

void main(){
  ex_Tex = in_Tex;
  gl_Position = projectionCamera*in_Model*vec4(in_Quad, 1.0);
}
