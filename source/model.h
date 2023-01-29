#ifndef SIMPLEHYDROLOGY_MODEL
#define SIMPLEHYDROLOGY_MODEL

/*
SimpleHydrology - model.h

Defines our rendering parameters,
and general visualization stuff.
*/

const int WIDTH = 1200;
const int HEIGHT = 800;

bool paused = true;
bool viewmap = false;
bool viewmomentum = false;

//Coloring
float steepness = 0.8;
glm::vec3 flatColor = glm::vec3(0.40, 0.60, 0.25);
glm::vec3 waterColor = glm::vec3(0.17, 0.40, 0.44);
glm::vec3 steepColor = glm::vec3(0.7);

//Lighting and Shading
glm::vec3 skyCol = glm::vec4(0.64, 0.75, 0.9, 1.0f);
glm::vec3 lightPos = glm::vec3(-100.0f, 75.0f, -150.0f);
//glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 0.9f);
glm::vec3 lightCol = glm::vec3(1.0f, 0.95f, 0.95f);
float lightStrength = 1.4;

//Matrix for Making Stuff Face Towards Light (Trees)
float rot = -1.0f * acos(glm::dot(glm::vec3(1, 0, 0), glm::normalize(glm::vec3(lightPos.x, 0, lightPos.z))));
glm::mat4 faceLight = glm::rotate(glm::mat4(1.0), rot , glm::vec3(0.0, 1.0, 0.0));

//Depth Map Rendering
glm::mat4 dp = glm::ortho<float>(-600, 600, -600, 600, 0, 800);
glm::mat4 dv = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0,1,0));
glm::mat4 bias = glm::mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
);
glm::mat4 dvp = dp*dv;
glm::mat4 dbvp = bias*dvp;

#endif
