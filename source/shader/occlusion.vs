#version 430 core

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec3 in_Tangent;
layout(location = 3) in vec3 in_Bitangent;

uniform mat4 vp;
uniform mat4 v;

out vec3 ex_Position;
out mat3 TBN;

void main(void) {

  ex_Position = in_Position;
	gl_Position = vp * vec4(in_Position, 1.0f);

  vec3 T =  normalize(in_Tangent);
  vec3 B =  normalize(in_Bitangent);
  vec3 N =  normalize(in_Normal);
  TBN = mat3(T, B, N);

}
