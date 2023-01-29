#version 430 core

in vec2 ex_Tex;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D gDepth;

uniform sampler2D ssaoTex;
uniform sampler2D shadowMap;
uniform sampler2D dischargeMap;

out vec4 fragColor;

uniform vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 lookDir;
uniform vec3 skyCol;
uniform float lightStrength;

uniform mat4 view;
uniform mat4 dbvp;

vec3 ex_Position;
vec4 ex_WorldPos;
vec3 ex_Normal;
vec4 ex_Shadow;

float gridSample(const int size){

  const float area = ((1 + 2*size)*(1 + 2*size));

  //Stuff
  float shadow = 0.0;
  float currentDepth = ex_Shadow.z;

  //Compute Bias
  float m = 1-dot(ex_Normal, normalize(lightPos));
  float bias = 0.0f;//mix(0.002, 0.2*m, pow(m, 5));

  for(int x = -size; x <= size; ++x){
      for(int y = -size; y <= size; ++y){
          float pcfDepth = texture(shadowMap, ex_Shadow.xy + vec2(x, y) / textureSize(shadowMap, 0)).r;
          shadow += currentDepth - 0.001 > pcfDepth ? 1.0 : 0.0;
      }
  }
  //Normalize
  shadow /= area;
  return shadow;
}

float shade(){

  float shadow = 0.0;
	const int size = 1;

  if(greaterThanEqual(ex_Shadow.xy, vec2(0.0f)) == bvec2(true) && lessThanEqual(ex_Shadow.xy, vec2(1.0f)) == bvec2(true))
    shadow = gridSample(size);


	return shadow;

}

vec3 blinnphong(){

  // Ambient (Factor)
  float ambient = 0.7;

  // Diffuse (Factor)

  vec3 lightpos = (view*vec4(lightPos, 1)).xyz;
  vec3 lightDir = normalize(lightpos);
  float diffuse  = 0.9*clamp(dot(ex_Normal, lightDir), 0.1, 0.9);

  // Specular Lighting (Factor)

  float ambientOcclusion = texture(ssaoTex, ex_Tex).r;
  float discharge = texture(dischargeMap, ex_WorldPos.xz/512).a;
  float specularStrength = 0.05 + 0.85*discharge;

  vec3 halfwayDir = normalize(lightDir + vec3(0,0,1));
  float spec = pow(max(dot(ex_Normal, halfwayDir), 0.0), 64);
  float specular = specularStrength * spec;

  // Multiply by Lightcolor

  return (ambientOcclusion*ambient + (1.0 - shade())*(diffuse + ambientOcclusion*specular))*lightCol;

}

void main() {

  // Extract Base Values

  ex_Position = texture(gPosition, ex_Tex).xyz;
  ex_Normal = texture(gNormal, ex_Tex).xyz;
  ex_WorldPos = inverse(view) *  vec4(ex_Position, 1.0f);
  ex_Shadow =  dbvp * ex_WorldPos;

  // Transform to Viewspace

//  fragColor = texture(shadowMap, ex_Tex);
  fragColor = texture(gColor, ex_Tex);
  fragColor = vec4(blinnphong()*fragColor.xyz, 1.0);

  // Depth-Fog

  float depthVal = clamp(texture(gDepth, ex_Tex).r, 0.0, 1.0);
  if(depthVal < 1)
    fragColor = mix(fragColor, vec4(1.0), 0.4*pow(depthVal, 2));
  else fragColor = vec4(skyCol, 1);

}