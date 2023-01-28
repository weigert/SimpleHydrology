#version 430 core
in vec2 ex_Tex;
out vec4 fragColor;

uniform sampler2D spriteTexture;
uniform sampler2D normalTexture;
uniform sampler2D shadowMap;

uniform mat4 faceLight;

uniform vec3 lightCol;
uniform float lightStrength;
uniform vec3 lightPos;
uniform vec3 lookDir;

in vec4 ex_Shadow;

float gridSample(int size){
  //Stuff
  float shadow = 0.0;
  float currentDepth = ex_Shadow.z;

  //Compute Bias
  float m = 0;//0;//1-dot(ex_Normal, normalize(lightPos));
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

vec4 shade(){
    //Shadow Value
    float shadow = 0.0;
    if(greaterThanEqual(ex_Shadow.xy, vec2(0.0f)) == bvec2(true) && lessThanEqual(ex_Shadow.xy, vec2(1.0f)) == bvec2(true))
      shadow = gridSample(1);

    //Sample the Shadow Value from Texture
    return vec4(vec3(1.0-shadow), 1.0f);
}

void main(){
  //Normal Mapping
  vec3 normal = (faceLight*vec4(texture(normalTexture, ex_Tex).xyz, 1.0)).xyz;
  float diffuse = clamp(dot(normal, normalize(lookDir)), 0.0, 1.0);

  vec4 color = texture(spriteTexture, ex_Tex);
  if(color.a == 0.0) discard;
  else fragColor = shade()*vec4(diffuse*color.xyz, 1.0);

}
