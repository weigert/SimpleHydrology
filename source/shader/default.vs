#version 430 core
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Tangent;
layout (location = 3) in vec3 in_Bitangent;

uniform sampler2D dischargeMap;
uniform sampler2D normalMap;

out vec3 ex_Position;
out vec3 ex_Normal;
out vec3 ex_Color;

const mat4 model = mat4(1.0f);
uniform mat4 view;
uniform mat4 proj;

uniform vec3 flatColor;
uniform vec3 waterColor;
uniform vec3 steepColor;

void main() {

    vec3 pos = in_Position;
  //  if(in_Position.y < 24)
  //    pos.y = 24;

    vec4 v_Position = view * model * vec4(pos, 1.0);
    ex_Position = v_Position.xyz;

    /*
    vec3 T = normalize(vec3(model * vec4(in_Tangent,   0.0)));
    vec3 B = normalize(vec3(model * vec4(in_Bitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(in_Normal,    0.0)));
    mat3 TBN = transpose(mat3(T, B, N));
    vec3 normal = texture(normalMap, vec2(ivec2(in_Position.xz)%ivec2(1))/1).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    ex_Normal = normalize(TBN*normal);
    */
    ex_Normal = in_Normal;
    //if(in_Position.y < 24)
    //  ex_Normal = vec3(0,1,0);

    ex_Normal = transpose(inverse(mat3(view * model))) * ex_Normal;

    gl_Position = proj * v_Position;

    // Color-Computation

    float steepness = 1.0f-pow(clamp((in_Normal.y-0.4)/0.6, 0.0, 1.0), 2);
    const ivec2 size = textureSize(dischargeMap, 0);
    float discharge = texture(dischargeMap, in_Position.xz/size).a;

    ex_Color = flatColor;
    ex_Color = mix(flatColor, steepColor, steepness*steepness);
    if(steepness > 0.6){
      ex_Color = steepColor;
    }
    ex_Color = mix(ex_Color, waterColor, discharge);
    //if(in_Position.y < 24)
    //  ex_Color = waterColor;

}
