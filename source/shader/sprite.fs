#version 430 core
in vec2 ex_Tex;
out vec4 fragColor;

uniform sampler2D spriteTexture;
uniform sampler2D normalTexture;

uniform mat4 faceLight;

uniform vec3 lightPos;
uniform vec3 lookDir;

void main(){
  //Normal Mapping
  vec3 normal = (faceLight*vec4(texture(normalTexture, ex_Tex).xyz, 1.0)).xyz;
  float diffuse = clamp(dot(normal, normalize(lookDir)), 0.0, 1.0);

  vec4 color = texture(spriteTexture, ex_Tex);
  if(color.a == 0.0) discard;
  else fragColor = vec4(diffuse*color.xyz, 1.0);

}
