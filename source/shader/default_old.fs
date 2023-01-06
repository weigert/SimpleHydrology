#version 430 core
//Lighting Settings
uniform vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 lookDir;
uniform float lightStrength;

//Sampler for the ShadowMap
uniform sampler2D shadowMap;

in vec4 ex_Color;
in vec3 ex_Normal;
in vec4 ex_Shadow;

out vec4 fragColor;

//Sample a grid..
float gridSample(int size){
  //Stuff
  float shadow = 0.0;
  float currentDepth = ex_Shadow.z;

  //Compute Bias
  float m = 1-dot(ex_Normal, normalize(lightPos));
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
    float shadow = 0.0;
		int size = 1;

    if(greaterThanEqual(ex_Shadow.xy, vec2(0.0f)) == bvec2(true) && lessThanEqual(ex_Shadow.xy, vec2(1.0f)) == bvec2(true))
      shadow = gridSample(size);

		return shadow;
}

vec4 phong() {

float diffuse = clamp(dot(ex_Normal, normalize(lightPos)), 0.2, 0.8);
float ambient = 0.3;
float spec = 0.7*pow(max(dot(normalize(lookDir), normalize(reflect(lightPos, ex_Normal))), 0.0), 32.0);

return vec4(lightCol*lightStrength*((1.0f-0.9*shade())*(diffuse + spec) + ambient ), 1.0f);

}


void main(void) {
  fragColor = vec4((phong()*ex_Color).xyz, 1.0f);
}
