#version 430 core

in vec2 ex_Tex;

uniform sampler2D dischargeMap;
uniform sampler2D momentumMap;

uniform bool view;

out vec4 fragColor;

void main(){

  fragColor = vec4(0,0,0,1);
  if(!view){
    vec4 mapColor = texture(dischargeMap, ex_Tex);
    fragColor = mix(fragColor, mapColor, mapColor.a);
    fragColor.a = 1.0;
  } else {
    fragColor = texture(momentumMap, ex_Tex);
  }
}
