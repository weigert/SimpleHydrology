#version 430 core

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;

//Uniforms
uniform mat4 vp;
uniform mat4 dbvp;

// We output the ex_Color variable to the next shader in the chain
out vec4 ex_Position;
out vec3 ex_Normal;
out vec4 ex_Color;
out vec4 ex_Shadow;

uniform sampler2D dischargeMap;

void main(void) {

  ex_Position = vp * vec4(in_Position, 1.0f);
  ex_Normal = in_Normal;
  ex_Shadow = dbvp * vec4(in_Position, 1.0f);
  gl_Position = ex_Position;

  const vec4 flatColor = vec4(0.40, 0.60, 0.25, 1);
  const vec4 waterColor = vec4(0.17, 0.40, 0.44, 1);
  const vec4 steepColor = vec4(0.7, 0.7, 0.7, 1);

  const float steepness = clamp((ex_Normal.y-0.4)/0.6, 0.0, 1.0);
  ex_Color = mix(steepColor, flatColor, pow(steepness, 2));
  ex_Color = mix(ex_Color, waterColor, texture(dischargeMap, in_Position.xz/512).a);

  /*
  ex_Color = flatColor;
  if(ex_Normal.y < 0.8)
    ex_Color = steepColor;
  ex_Color = mix(ex_Color, waterColor, texture(dischargeMap, in_Position.xz/512).a);
  */

  /*
    if(ex_Position.y < 8){
      ex_Position.y = 8;
      ex_Color = vec4(0.17, 0.40, 0.44,1);
    }
  */

}
