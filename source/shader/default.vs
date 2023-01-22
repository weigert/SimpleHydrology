#version 430 core

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec4 in_Color;

//Lighting
uniform vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 lookDir;
uniform float lightStrength;

//Uniforms
uniform mat4 vp;
uniform mat4 dbvp;

uniform sampler2D shadowMap;

// We output the ex_Color variable to the next shader in the chain
flat out vec4 ex_Color;
out vec4 ex_Shadow;



float gridSample(int size){
  //Stuff
  float shadow = 0.0;
  float currentDepth = ex_Shadow.z;

  //Compute Bias
  float m = 1-dot(in_Normal, normalize(lightPos));
  float bias = mix(0.002, 0.2*m, pow(m, 5));

  for(int x = -size; x <= size; ++x){
      for(int y = -size; y <= size; ++y){
          float pcfDepth = texture(shadowMap, ex_Shadow.xy + vec2(x, y) / textureSize(shadowMap, 0)).r;
          shadow += currentDepth - 0.001 > pcfDepth ? 1.0 : 0.0;
      }
  }
  //Normalize
  shadow/=12.0;
  return shadow;
}

float shade(){

/*
    float shadow = 0.0;
		int size = 1;

    if(greaterThanEqual(ex_Shadow.xy, vec2(0.0f)) == bvec2(true) && lessThanEqual(ex_Shadow.xy, vec2(1.0f)) == bvec2(true))
      shadow = gridSample(size);
*/
		return 0.0;//shadow;
}

vec4 phong() {

float diffuse = clamp(dot(in_Normal, normalize(lightPos)), 0.1, 0.9);
float ambient = 0.3;
float spec = 0.1*pow(max(dot(normalize(lookDir), normalize(reflect(lightPos, in_Normal))), 0.0), 32.0);

return vec4(lightCol*lightStrength*((1.0f-0.9*shade())*(diffuse + spec) + ambient ), 1.0f);

}

void main(void) {

  vec3 ex_Position = in_Position;
  ex_Color = vec4((phong()*in_Color).xyz, 1.0f);

  if(ex_Position.y < 8){
    ex_Position.y = 8;
    ex_Color = vec4(0.17, 0.40, 0.44,1);
  }

	ex_Shadow = dbvp * vec4(ex_Position, 1.0f);
	gl_Position = vp * vec4(ex_Position, 1.0f);
}
