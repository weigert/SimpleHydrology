#version 430 core
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Tangent;
layout (location = 3) in vec3 in_Bitangent;

uniform sampler2D dischargeMap;

out vec3 ex_Position;
out vec3 ex_Normal;
out vec4 ex_Color;

const mat4 model = mat4(1.0f);
uniform mat4 view;
uniform mat4 proj;

void main() {

    const vec4 v_Position = view * model * vec4(in_Position, 1.0);
    ex_Position = v_Position.xyz;
    ex_Normal = transpose(inverse(mat3(view * model))) * in_Normal;
    gl_Position = proj * v_Position;

    // Color-Computation
    const vec4 flatColor = vec4(0.40, 0.60, 0.25, 1);
    const vec4 waterColor = vec4(0.17, 0.40, 0.44, 1);
    const vec4 steepColor = vec4(vec3(0.7), 1);

    float steepness = 1.0f-pow(clamp((in_Normal.y-0.4)/0.6, 0.0, 1.0), 2);
    const ivec2 size = textureSize(dischargeMap, 0);
    float discharge = texture(dischargeMap, in_Position.xz/size).a;

    ex_Color = mix(flatColor, steepColor, steepness);
    ex_Color = mix(ex_Color, waterColor, discharge);

}
