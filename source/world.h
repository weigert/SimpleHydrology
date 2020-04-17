#include "water.h"

class World{
public:
  //Constructor
  void generate();                        //Initialize Heightmap
  void erode(int cycles);                 //Perform n erosion cycles

  int SEED = 10;
  std::chrono::milliseconds tickLength = std::chrono::milliseconds(1000);
  glm::vec2 dim = glm::vec2(256, 256);  //Size of the heightmap array
  bool updated = false;                 //Flag for remeshing

  double scale = 60.0;                  //"Physical" Height scaling of the map
  double heightmap[256*256] = {0.0};    //Flat Array

  double waterpath[256*256] = {0.0};
  double waterpool[256*256] = {0.0};

  //Erosion Steps
  bool active = false;
  int remaining = 200000;
  int erosionstep = 1000;
};

/*
===================================================
          WORLD GENERATING FUNCTIONS
===================================================
*/

void World::generate(){
  std::cout<<"Generating New World"<<std::endl;
  SEED = time(NULL);
  std::cout<<"Seed: "<<SEED<<std::endl;
  //Seed the Random Generator
  srand(SEED);

  std::cout<<"... generating height ..."<<std::endl;

  //Initialize Heightmap
  noise::module::Perlin perlin;

  //Mountainy:
  perlin.SetOctaveCount(8);
  perlin.SetFrequency(0.8);
  perlin.SetPersistence(0.6);

  float min, max = 0.0;
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = perlin.GetValue((i/256)*(1.0/dim.x), (i%256)*(1.0/dim.y), SEED);
    if(heightmap[i] > max) max = heightmap[i];
    if(heightmap[i] < min) min = heightmap[i];
  }
  //Normalize
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = (heightmap[i] - min)/(max - min);
  }

  //Construct all Triangles...
  updated = true;
}

/*
===================================================
          HYDRAULIC EROSION FUNCTIONS
===================================================
*/

void World::erode(int cycles){

  //Do a series of iterations!
  for(int i = 0; i < cycles; i++){

    //Spawn New Particle
    glm::vec2 newpos = glm::vec2(rand()%(int)dim.x, rand()%(int)dim.y);
    Drop drop(newpos);

    int spill = 5;
    while(drop.volume > drop.minVol && spill != 0){

      drop.process(heightmap, waterpath, waterpool, dim, scale);

      if(drop.volume > drop.minVol)
        drop.flood(heightmap, waterpool);

      spill--;
    }

  }

}

/*
===================================================
                MESHING FUNCTION
===================================================
*/

int WIDTH = 1000;
int HEIGHT = 1000;

bool paused = true;

float zoom = 0.2;
float zoomInc = 0.01;

//Rotation and View
float rotation = 0.0f;
glm::vec3 cameraPos = glm::vec3(50, 50, 50);
glm::vec3 lookPos = glm::vec3(0, 0, 0);
glm::mat4 camera = glm::lookAt(cameraPos, lookPos, glm::vec3(0,1,0));
glm::mat4 projection = glm::ortho(-(float)WIDTH*zoom, (float)WIDTH*zoom, -(float)HEIGHT*zoom, (float)HEIGHT*zoom, -800.0f, 500.0f);

//Shader Stuff
float steepness = 0.8;
glm::vec3 flatColor = glm::vec3(0.27, 0.64, 0.27);
glm::vec3 steepColor = glm::vec3(0.7);
//glm::vec3 flatColor = glm::vec3(1.0, 0.709, 0.329);
//glm::vec3 steepColor = glm::vec3(0.847, 0.714, 0.592);
glm::vec3 waterColor = glm::vec3(0.086, 0.435, 0.494);

//Lighting and Shading
glm::vec3 skyCol = glm::vec4(0.3, 0.3f, 1.0f, 1.0f);
glm::vec3 lightPos = glm::vec3(-100.0f, 100.0f, -150.0f);
glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 0.9f);
float lightStrength = 1.4;
glm::mat4 depthModelMatrix = glm::mat4(1.0);
glm::mat4 depthProjection = glm::ortho<float>(-300, 300, -300, 300, 0, 800);
glm::mat4 depthCamera = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0,1,0));

glm::mat4 biasMatrix = glm::mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
);

World world;

std::function<void(Model* m)> constructor = [&](Model* m){
  //Clear the Containers
  m->indices.clear();
  m->positions.clear();
  m->normals.clear();
  m->colors.clear();

  //Loop over all positions and add the triangles!
  for(int i = 0; i < world.dim.x-1; i++){
    for(int j = 0; j < world.dim.y-1; j++){

      //Get Index
      int ind = i*world.dim.y+j;

      //Add to Position Vector
      glm::vec3 a = glm::vec3(i, world.scale*world.heightmap[ind], j);
      glm::vec3 b = glm::vec3(i+1, world.scale*world.heightmap[ind+256], j);
      glm::vec3 c = glm::vec3(i, world.scale*world.heightmap[ind+1], j+1);
      glm::vec3 d = glm::vec3(i+1, world.scale*world.heightmap[ind+256+1], j+1);

      //Add the Pool Height
      bool water = (world.waterpool[ind] > 0.0);
      a += glm::vec3(0.0, world.scale*world.waterpool[ind], 0.0);
      b += glm::vec3(0.0, world.scale*world.waterpool[ind+256], 0.0);
      c += glm::vec3(0.0, world.scale*world.waterpool[ind+1], 0.0);
      d += glm::vec3(0.0, world.scale*world.waterpool[ind+256+1], 0.0);

      //UPPER TRIANGLE

      //Get the Color of the Ground (Water vs. Flat)
      glm::vec3 color;
      float p = world.waterpath[ind];
      if(water) color = waterColor;
      else color = glm::mix(flatColor, waterColor, ease::sharpen(p,2));

      //Add Indices
      m->indices.push_back(m->positions.size()/3+0);
      m->indices.push_back(m->positions.size()/3+1);
      m->indices.push_back(m->positions.size()/3+2);

      m->positions.push_back(a.x);
      m->positions.push_back(a.y);
      m->positions.push_back(a.z);
      m->positions.push_back(b.x);
      m->positions.push_back(b.y);
      m->positions.push_back(b.z);
      m->positions.push_back(c.x);
      m->positions.push_back(c.y);
      m->positions.push_back(c.z);

      glm::vec3 n1 = glm::normalize(glm::cross(a-b, c-b));

      for(int i = 0; i < 3; i++){
        m->normals.push_back(n1.x);
        m->normals.push_back(n1.y);
        m->normals.push_back(n1.z);

        //Add the Color!
        if(n1.y < steepness){
          m->colors.push_back(steepColor.x);
          m->colors.push_back(steepColor.y);
          m->colors.push_back(steepColor.z);
          m->colors.push_back(1.0);
        }
        else{
          m->colors.push_back(color.x);
          m->colors.push_back(color.y);
          m->colors.push_back(color.z);
          m->colors.push_back(1.0);
        }

      }

      //Lower Triangle

      m->indices.push_back(m->positions.size()/3+0);
      m->indices.push_back(m->positions.size()/3+1);
      m->indices.push_back(m->positions.size()/3+2);

      m->positions.push_back(d.x);
      m->positions.push_back(d.y);
      m->positions.push_back(d.z);
      m->positions.push_back(c.x);
      m->positions.push_back(c.y);
      m->positions.push_back(c.z);
      m->positions.push_back(b.x);
      m->positions.push_back(b.y);
      m->positions.push_back(b.z);

      glm::vec3 n2 = glm::normalize(glm::cross(d-c, b-c));

      for(int i = 0; i < 3; i++){
        m->normals.push_back(n2.x);
        m->normals.push_back(n2.y);
        m->normals.push_back(n2.z);

        if(n2.y < steepness){
          m->colors.push_back(steepColor.x);
          m->colors.push_back(steepColor.y);
          m->colors.push_back(steepColor.z);
          m->colors.push_back(1.0);
        }
        else{
          m->colors.push_back(color.x);
          m->colors.push_back(color.y);
          m->colors.push_back(color.z);
          m->colors.push_back(1.0);
        }

      }
    }
  }
};

std::function<void()> eventHandler = [&](){

  if(!Tiny::event.scroll.empty()){

    if(Tiny::event.scroll.back().wheel.y > 0.99 && zoom <= 0.3){
      zoom += zoomInc;
      projection = glm::ortho(-(float)WIDTH*zoom, (float)WIDTH*zoom, -(float)HEIGHT*zoom, (float)HEIGHT*zoom, -800.0f, 500.0f);
    }
    else if(Tiny::event.scroll.back().wheel.y < -0.99 && zoom > 0.005){
      zoom -= zoomInc;
      projection = glm::ortho(-(float)WIDTH*zoom, (float)WIDTH*zoom, -(float)HEIGHT*zoom, (float)HEIGHT*zoom, -800.0f, 500.0f);
    }
    else if(Tiny::event.scroll.back().wheel.x < -0.8){
      rotation += 1.5f;
      camera = glm::rotate(camera, glm::radians(1.5f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if(Tiny::event.scroll.back().wheel.x > 0.8){
      rotation -= 1.5f;
      camera = glm::rotate(camera, glm::radians(-1.5f), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    //Adjust Stuff
    if(rotation < 0.0) rotation = 360.0 + rotation;
    else if(rotation > 360.0) rotation = rotation - 360.0;
    camera = glm::rotate(glm::lookAt(cameraPos, lookPos, glm::vec3(0,1,0)), glm::radians(rotation), glm::vec3(0,1,0));
    Tiny::event.scroll.pop_back();
  }

  if(!Tiny::event.keys.empty()){

    if(Tiny::event.keys.back().key.keysym.sym == SDLK_p){
      paused = !paused;
    }

    if(Tiny::event.keys.back().key.keysym.sym == SDLK_UP){
      cameraPos += glm::vec3(0, 5, 0);
      camera = glm::lookAt(cameraPos, lookPos, glm::vec3(0,1,0));
    }

    if(Tiny::event.keys.back().key.keysym.sym == SDLK_DOWN){
      cameraPos -= glm::vec3(0, 5, 0);
      camera = glm::lookAt(cameraPos, lookPos, glm::vec3(0,1,0));
    }

    //Remove the guy
    Tiny::event.keys.pop_back();
  }
};

std::function<glm::vec4(double)> pathColor = [](double t){
  return glm::mix(glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(0.8, 0.0, 0.0, 1.0), t);
};

std::function<glm::vec4(double)> poolColor = [](double t){
  return glm::mix(glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), t);
};
