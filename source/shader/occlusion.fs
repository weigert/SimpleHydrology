#version 430 core

in vec3 ex_Position;
in mat3 TBN;

uniform sampler2D positionTexture;

uniform mat4 p;
uniform mat4 v;
uniform mat4 vp;

out vec4 fragColor;

void main(void) {

  float occlusion = 0.0;
  float radius = 1.0f;

  const int kernelSize = 5;

  const vec3[kernelSize] samples = {
    normalize(vec3(   0, 0, 1)),
    normalize(vec3( 0.1, 0, 1)),
    normalize(vec3(   0, 0.1, 1)),
    normalize(vec3(-0.1, 0, 1)),
    normalize(vec3(   0, -0.1, 1)),
    /*
    normalize(vec3(   0, 1, -0.1)),
    normalize(vec3( 0.2, 1, 0)),
    normalize(vec3(   0, 1, 0.2)),
    normalize(vec3(-0.2, 1, 0)),
    normalize(vec3(   0, 1, -0.2))
    */
  };

  for(int i = 0; i < kernelSize; i++){

    vec3 _sample = samples[i];

    vec4 viewpos = v * vec4(ex_Position + radius * TBN * _sample, 1);

    vec3 samplepos = viewpos.xyz;

    // 3: Compute the Offset Position, Transform to Range

    vec4 offset = vec4(samplepos, 1);
    offset      = p * offset;             // from view to clip-space
    offset.xyz /= offset.w;
    offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

    // Position value (view space)
    fragColor = texture(positionTexture, offset.xy);

    float offsetDepth = texture(positionTexture, offset.xy).z;

    // The actual depth value
  //  fragColor = vec4(vec3(offsetDepth), 1);

    vec4 nsamplepos = p*vec4(samplepos, 1);
    nsamplepos.xyz = 0.5+0.5*nsamplepos.xyz;

  //  fragColor = vec4(nsamplepos.xyz, 1);
    fragColor = vec4(vec3(10*abs(offset.z - offsetDepth)), 1);

    float rangeCheck = smoothstep(0.0, 1.0, radius / abs(offset.z - offsetDepth));
    occlusion += (offsetDepth >= offset.z + 0.1 ? 1.0 : 0.0) * rangeCheck;

  }

  occlusion /= kernelSize;
  //  occlusion = 1.0 - occlusion/float(kernelSize);
  //  fragColor = vec4(vec3(occlusion), 1);

  /*

  // Visualize the Position Texture
  vec4 fragpos = vp*vec4(ex_Position, 1);
  fragColor = texture(positionTexture, 0.5 + 0.5*fragpos.xy);

  */

  /*

  // Visualize the Computed Position

  vec4 fragpos = vp*vec4(ex_Position, 1);
  fragColor = vec4(0.5+0.5*fragpos.xyz, 1);

  */

}
