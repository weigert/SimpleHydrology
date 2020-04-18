#version 130
in vec2 ex_Tex;
out vec4 fragColor;

uniform sampler2D spriteTexture;

uniform vec3 lightPos;
uniform vec3 lookDir;

void main(){
  float diffuse = clamp(dot(normalize(lookDir.xz), normalize(lightPos.xz)), 0.2, 0.8);

  vec4 color = texture(spriteTexture, ex_Tex);
  if(color.a == 0.0) discard;
  else fragColor = vec4(diffuse*color.xyz, 1.0);
}
