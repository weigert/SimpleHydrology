#version 430 core

in vec2 ex_Tex;
out vec4 fragColor;

uniform sampler2D imageTexture;
uniform sampler2D depthTexture;
uniform sampler2D heightTexture;

uniform bool distancefog;
uniform vec3 skycolor;

void main(){

    fragColor = texture(imageTexture, ex_Tex);

    /*
    int foundedge = 0;
    vec4 a0 = textureOffset(heightTexture, ex_Tex, ivec2( 0, 0));

    vec4 a1 = textureOffset(heightTexture, ex_Tex, ivec2(-1, 0));
    //vec4 a2 = textureOffset(heightTexture, ex_Tex, ivec2( 1, 0));
    vec4 a3 = textureOffset(heightTexture, ex_Tex, ivec2( 0,-1));
    //vec4 a4 = textureOffset(heightTexture, ex_Tex, ivec2( 0, 1));

    if(a1 != a0) foundedge++;
    //if(a2 != a0) foundedge++;
    if(a3 != a0) foundedge++;
  //  if(a4 != a0) foundedge++;

    if(foundedge > 0)
      fragColor = vec4(0,0,0,1);

    float depthVal = 1.0f-clamp(texture(depthTexture, ex_Tex).r, 0.0, 1.0);

//    if(depthVal < 1.0 && distancefog)
//      fragColor = mix(fragColor, vec4(vec3(1), 1), pow(2*depthVal-0.55,2));
  //    //If it is a visible thing...
      fragColor = mix(fragColor, vec4(1.0), pow(3*(depthVal-0.55),2));  //White Fog Color!
      */

}
