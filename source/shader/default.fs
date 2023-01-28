#version 430 core

in vec4 ex_Position;
in vec3 ex_Normal;
in vec4 ex_Color;
in vec4 ex_Shadow;
in vec2 ex_Surface;
in mat3 TBN;

in float steepness;
in float discharge;

//Lighting
uniform vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 lookDir;
uniform float lightStrength;

out vec4 fragColor;

uniform sampler2D shadowMap;
uniform sampler2D normalMap;

float gridSample(const int size){

  const float area = ((1 + 2*size)*(1 + 2*size));

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
  shadow /= area;//12.0;
  return shadow;
}

float shade(){

    float shadow = 0.0;
		const int size = 1;

    if(greaterThanEqual(ex_Shadow.xy, vec2(0.0f)) == bvec2(true) && lessThanEqual(ex_Shadow.xy, vec2(1.0f)) == bvec2(true))
      shadow = gridSample(size);

		return shadow;
}



vec3 blinnphong(vec3 normal){

  // Ambient (Factor)

  float ambient = 0.6;

  // Diffuse (Factor)

  vec3 lightDir = normalize(lightPos - ex_Position.xyz);
  float diffuse  = 0.7*clamp(dot(normal, lightDir), 0.1, 0.9);

  // Specular Lighting (Factor)

  float specularStrength = 0.05 + 0.85*discharge;

  vec3 viewDir = normalize(lookDir - ex_Position.xyz);
  vec3 halfwayDir = normalize(lightDir + viewDir);

  float spec = pow(max(dot(normal, halfwayDir), 0.0), 64);
  float specular = specularStrength * spec;

  // Multiply by Lightcolor

  return (ambient + (1.0 - shade())*(diffuse + specular))*lightCol;

}

void main() {

   // Vertex-Shader Color

   fragColor = ex_Color;

   // Normal-Map Vector

   vec3 normal = ex_Normal;

   /*
   vec3 texnormal = texture(normalMap, mod(ex_Surface, vec2(16))/16).xyz;
   texnormal = TBN*normalize(texnormal * 2.0 - 1.0);
   normal = mix(normal, texnormal, steepness);
   */

   fragColor = vec4(blinnphong(normal)*fragColor.xyz, 1.0);
  // fragColor = vec4(blinnphong(ex_Normal)*fragColor.xyz, 1.0);

//   fragColor = vec4(ex_Normal, 1);

   // fragColor = ;



}
