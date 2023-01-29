#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColor;

in vec3 ex_Position;
in vec3 ex_Normal;
in vec4 ex_Color;

void main() {

    gPosition = ex_Position;
    gNormal = normalize(ex_Normal);
    gColor = ex_Color;

}
