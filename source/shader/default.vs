#version 130
in vec3 in_Position;
in vec3 in_Normal;
in vec4 in_Color;

//Lighting
uniform vec3 lightCol;
uniform vec3 lightPos;
uniform vec3 lookDir;
uniform float lightStrength;

//Color Stuff
uniform vec3 flatColor;
uniform vec3 steepColor;
uniform float steepness;

//Uniforms
uniform mat4 model;
uniform mat4 projectionCamera;
uniform mat4 dbmvp;

// We output the ex_Color variable to the next shader in the chain
out vec4 ex_Color;
out vec3 ex_Normal;
out vec2 ex_Position;
out vec4 ex_Shadow;
out vec3 ex_FragPos;

vec4 gouraud(){
	//Color Calculations - Per Vertex! Not Fragment.
	float diffuse = clamp(dot(in_Normal, normalize(lightPos)), 0.1, 0.9);
	float ambient = 0.1;
	float spec = 0.8*pow(max(dot(normalize(lookDir), normalize(reflect(lightPos, in_Normal))), 0.0), 32.0);

	return vec4(lightCol*lightStrength*(diffuse + ambient + spec), 1.0f);
}

void main(void) {
	vec3 inPos = in_Position;
	ex_FragPos = (model * vec4(inPos, 1.0f)).xyz;
	ex_Shadow = dbmvp * vec4(ex_FragPos, 1.0f);
	gl_Position = projectionCamera * vec4(ex_FragPos, 1.0f);
	ex_Position = ((gl_Position.xyz / gl_Position.w).xy * 0.5 + 0.5 );
	ex_Normal = in_Normal;
	ex_Color = gouraud()*in_Color;
}
