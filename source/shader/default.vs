#version 430 core

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec4 in_Color;

//Lighting
uniform vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 lookDir;
uniform float lightStrength;

//Uniforms
uniform mat4 vp;
uniform mat4 dbvp;

// We output the ex_Color variable to the next shader in the chain
out vec4 ex_Color;
out vec3 ex_Normal;
out vec4 ex_Shadow;

vec4 gouraud(){
	//Color Calculations - Per Vertex! Not Fragment.
	float diffuse = clamp(dot(in_Normal, normalize(lightPos)), 0.1, 0.9);
	float ambient = 0.1;
	float spec = 0.8*pow(max(dot(normalize(lookDir), normalize(reflect(lightPos, in_Normal))), 0.0), 32.0);

	return vec4(lightCol*lightStrength*(diffuse + ambient + spec), 1.0f);
}

void main(void) {
	ex_Shadow = dbvp * vec4(in_Position, 1.0f);
	gl_Position = vp * vec4(in_Position, 1.0f);
	ex_Normal = in_Normal;
	ex_Color = gouraud()*in_Color;
}
