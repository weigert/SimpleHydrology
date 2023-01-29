#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;

in vec3 ex_Position;
in vec3 ex_Normal;

void main() {

    gPosition = ex_Position;
    gNormal = normalize(ex_Normal);

}
