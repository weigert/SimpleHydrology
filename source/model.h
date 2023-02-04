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
glm::vec3 flatColor = glm::vec3(50, 81, 33)/255.0f;
glm::vec3 steepColor = glm::vec3(115, 115, 95)/255.0f;
glm::vec3 waterColor = glm::vec3(92, 133, 142)/255.0f;
glm::vec3 skyCol = glm::vec3(173, 183, 196)/255.0f;
glm::vec3 treeColor = glm::vec3(70, 90, 50)/255.0f;

//Lighting and Shading
glm::vec3 lightPos = normalize(glm::vec3(50.0f, 25.0f, -50.0f));
glm::vec3 lightCol = glm::vec3(1.0f, 0.95f, 0.95f);
float lightStrength = 1.4;

//Depth Map Rendering

glm::vec3 worldcenter = glm::vec3(quad::res.x/2, quad::mapscale/2, quad::res.y/2);

//Matrix for Making Stuff Face Towards Light (Trees)
glm::mat4 dp = glm::ortho<float>(-400, 400, -400, 400, -400, 400);
glm::mat4 dv = glm::lookAt(worldcenter + lightPos, worldcenter, glm::vec3(0,1,0));
glm::mat4 bias = glm::mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
);
glm::mat4 dvp = dp*dv;
glm::mat4 dbvp = bias*dvp;

#endif
