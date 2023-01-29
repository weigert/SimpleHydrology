#version 430 core
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Tangent;
layout (location = 3) in vec3 in_Bitangent;

out vec3 ex_Position;
out vec3 ex_Normal;

const mat4 model = mat4(1.0f);
uniform mat4 view;
uniform mat4 proj;

void main() {

    const vec4 v_Position = view * model * vec4(in_Position, 1.0);
    ex_Position = v_Position.xyz;
    ex_Normal = transpose(inverse(mat3(view * model))) * in_Normal;
    gl_Position = proj * v_Position;

}
