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
glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 0.9f);
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

//Vertex Pool and Surface Meshing
uint* section = NULL;

void indexmap(Vertexpool<Vertex>& vertexpool, World& world){

  for(int i = 0; i < WSIZE-1; i++){
  for(int j = 0; j < WSIZE-1; j++){

    vertexpool.indices.push_back(math::flatten(ivec2(i, j), World::dim));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), World::dim));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), World::dim));

    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), World::dim));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), World::dim));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j+1), World::dim));

  }}

  vertexpool.resize(section, vertexpool.indices.size());
  vertexpool.index();
  vertexpool.update();

}

//Use the Position Hash (Generates Hash from Index)
std::hash<std::string> position_hash;
double hashrand(int i){
  return (double)(position_hash(std::to_string(i))%1000)/1000.0;
}

void updatemap(Vertexpool<Vertex>& vertexpool, World& world){

  for(int i = 0; i < WSIZE; i++)
  for(int j = 0; j < WSIZE; j++){

    float height = SCALE*world.get(ivec2(i, j)).height;

    float p = World::getDischarge(vec2(i, j));
    glm::vec3 color = flatColor;

    glm::vec3 normal = world.normal(vec2(i, j));
    if(normal.y < steepness)
      color = steepColor;

    color = glm::mix(color, waterColor, p);

    double t = hashrand(math::flatten(ivec2(i, j), World::dim));
    color = glm::mix(color, vec3(0), 0.3*t*(1.0f-p));

    vertexpool.fill(section, math::flatten(ivec2(i, j), World::dim),
      glm::vec3(i, height, j),
      normal,
      vec4(color, 1.0f)
    );

  }

}

#endif
