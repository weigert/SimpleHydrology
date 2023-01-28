#version 430 core

in vec4 ex_Position;
in vec3 ex_Normal;
in vec4 ex_Color;
in vec4 ex_Shadow;

//Lighting
uniform vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 lookDir;
uniform float lightStrength;

out vec4 fragColor;

/*
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


    float shadow = 0.0;
		int size = 1;

    if(greaterThanEqual(ex_Shadow.xy, vec2(0.0f)) == bvec2(true) && lessThanEqual(ex_Shadow.xy, vec2(1.0f)) == bvec2(true))
      shadow = gridSample(size);

		return shadow;
}

*/

vec3 blinnphong(){

  // Ambient (Factor)

  float ambient = 0.5;

  // Diffuse (Factor)

  vec3 lightDir = normalize(lightPos - ex_Position.xyz);
  float diffuse = clamp(dot(ex_Normal, lightDir), 0.1, 0.9);

  // Specular Lighting (Factor)

  float specularStrength = 0.15;

  vec3 viewDir = normalize(lookDir - ex_Position.xyz);
  vec3 halfwayDir = normalize(lightDir + viewDir);

  float spec = pow(max(dot(ex_Normal, halfwayDir), 0.0), 32);
  float specular = specularStrength * spec;

  // Multiply by Lightcolor

  return (ambient + diffuse + specular)*lightCol;

}

void main() {

   fragColor = vec4(blinnphong()*ex_Color.xyz, 1.0);

}
