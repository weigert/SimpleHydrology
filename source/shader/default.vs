#version 430 core

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec3 in_Tangent;
layout(location = 3) in vec3 in_Bitangent;

//Uniforms
uniform mat4 vp;
uniform mat4 dbvp;

// We output the ex_Color variable to the next shader in the chain
out vec4 ex_Position;
out vec2 ex_Surface;
out vec3 ex_Normal;
out vec4 ex_Color;
out vec4 ex_Shadow;
out mat3 TBN;

out float steepness;
out float discharge;

uniform sampler2D dischargeMap;

void main(void) {

  // World-Space Position
  ex_Position = vp * vec4(in_Position, 1.0f);
  gl_Position = ex_Position;

  // Shadow, Normal, Surface Position
  ex_Shadow = dbvp * vec4(in_Position, 1.0f);
  ex_Normal = in_Normal;
  ex_Surface = in_Position.xz;

  // Tangent Space Transformation Matrix
  vec3 T = normalize(in_Tangent);
  vec3 B = normalize(in_Bitangent);
  vec3 N = normalize(in_Normal);
  TBN = mat3(T, B, N);

  // Color-Computation
  const vec4 flatColor = vec4(0.40, 0.60, 0.25, 1);
  const vec4 waterColor = vec4(0.17, 0.40, 0.44, 1);
  const vec4 steepColor = vec4(vec3(0.7), 1);

  steepness = 1.0f-pow(clamp((ex_Normal.y-0.4)/0.6, 0.0, 1.0), 2);
  discharge = texture(dischargeMap, in_Position.xz/512).a;

  ex_Color = mix(flatColor, steepColor, steepness);
  ex_Color = mix(ex_Color, waterColor, discharge);

  ex_Normal = mix(ex_Normal, vec3(0,1,0), discharge);



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
