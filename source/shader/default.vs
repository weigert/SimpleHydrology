#version 430 core

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec4 in_Color;

//Uniforms
uniform mat4 vp;
uniform mat4 dbvp;

// We output the ex_Color variable to the next shader in the chain
out vec4 ex_Position;
out vec3 ex_Normal;
out vec4 ex_Color;
out vec4 ex_Shadow;

void main(void) {

  /*
    if(ex_Position.y < 8){
      ex_Position.y = 8;
      ex_Color = vec4(0.17, 0.40, 0.44,1);
    }
  */

  ex_Position = vp * vec4(in_Position, 1.0f);
  ex_Color = in_Color;
  ex_Normal = in_Normal;
  ex_Shadow = dbvp * vec4(in_Position, 1.0f);
  gl_Position = ex_Position;

}
