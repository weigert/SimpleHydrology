#ifndef SIMPLEHYDROLOGY_WORLD
#define SIMPLEHYDROLOGY_WORLD

/*
SimpleHydrology - world.h

Defines our main storage buffers,
world updating functions for erosion
and vegetation.
*/

class World {

public:

  int SEED = 0;
  static glm::ivec2 dim;                      //Size of the Map

  // Storage Arrays

  static float heightmap[WSIZE*WSIZE];        //Flat Array
  static float discharge[WSIZE*WSIZE];        //Discharge Storage (Rivers)
  static float momentumx[WSIZE*WSIZE];        //Momentum X Storage (Rivers)
  static float momentumy[WSIZE*WSIZE];        //Momentum Y Storage (Rivers)

  // Main Update Methods

  void generate();                            // Initialize Heightmap
  static glm::vec3 normal(int index);         // Compute Surface Normal
  void erode(int cycles);                     // Erosion Update Step
  static void cascade(vec2 pos);              // Perform Sediment Cascade

};

glm::ivec2 World::dim = glm::vec2(WSIZE, WSIZE);

float World::heightmap[WSIZE*WSIZE] = {0.0};    //Flat Array
float World::discharge[WSIZE*WSIZE] = {0.0};    //Water Path Storage (Rivers)
float World::momentumx[WSIZE*WSIZE] = {0.0};    //Momentum X Storage (Rivers)
float World::momentumy[WSIZE*WSIZE] = {0.0};    //Momentum Y Storage (Rivers)

#include "vegetation.h"
#include "water.h"

/*
===================================================
          WORLD GENERATING FUNCTIONS
===================================================
*/

glm::vec3 World::normal(int index){

  //Two large triangels adjacent to the plane (+Y -> +X) (-Y -> -X)
  glm::vec3 n = glm::cross(glm::vec3(0.0, SCALE*(heightmap[index+1]-heightmap[index]), 1.0), glm::vec3(1.0, SCALE*(heightmap[index+dim.y]-heightmap[index]), 0.0));
  n += glm::cross(glm::vec3(0.0, SCALE*(heightmap[index-1]-heightmap[index]), -1.0), glm::vec3(-1.0, SCALE*(heightmap[index-dim.y]-heightmap[index]), 0.0));

  //Two Alternative Planes (+X -> -Y) (-X -> +Y)
  n += glm::cross(glm::vec3(1.0, SCALE*(heightmap[index+dim.y]-heightmap[index]), 0.0), glm::vec3(0.0, SCALE*(heightmap[index-1]-heightmap[index]), -1.0));
  n += glm::cross(glm::vec3(-1.0, SCALE*(heightmap[index-dim.y]-heightmap[index]), 0.0), glm::vec3(0.0, SCALE*(heightmap[index+1]-heightmap[index]), 1.0));

  return glm::normalize(n);

}

void World::generate(){

  std::cout<<"Generating New World"<<std::endl;
  if(SEED == 0) SEED = time(NULL);

  std::cout<<"Seed: "<<SEED<<std::endl;
  srand(SEED);

  std::cout<<"... generating height ..."<<std::endl;

  //Initialize Heightmap
  noise::module::Perlin perlin;

  //Mountainy:
  perlin.SetOctaveCount(8);
  perlin.SetFrequency(FREQUENCY);
  perlin.SetPersistence(0.6);

  float min, max = 0.0;
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = perlin.GetValue((i/dim.y)*(1.0/dim.x), (i%dim.y)*(1.0/dim.y), SEED);
    if(heightmap[i] > max) max = heightmap[i];
    if(heightmap[i] < min) min = heightmap[i];
  }
  //Normalize
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = (heightmap[i] - min)/(max - min);//+1.0f*((float)((i/dim.y)*(i/dim.y))/dim.x/dim.x)*((float)((i%dim.y)*(i%dim.y))/dim.y/dim.y);
  }
}

/*
===================================================
          HYDRAULIC EROSION FUNCTIONS
===================================================
*/
void World::erode(int cycles){

  //Track the Movement of all Particles
  float track[dim.x*dim.y] = {0.0f};
  float mx[dim.x*dim.y] = {0.0f};
  float my[dim.x*dim.y] = {0.0f};

  //Do a series of iterations!
  for(int i = 0; i < cycles; i++){

    //Spawn New Particle
    glm::vec2 newpos = glm::vec2(rand()%(int)dim.x, rand()%(int)dim.y);
    Drop drop(newpos);

    while(drop.descend(track, mx, my, SCALE));

  }

  //Update Path
  float lrate = 0.1;
  for(int i = 0; i < dim.x*dim.y; i++){

    discharge[i] = (1.0-lrate)*discharge[i] + lrate*track[i];///(1.0f + 50.0f*track[i]);

    momentumx[i] = (1.0-lrate)*momentumx[i] + lrate*mx[i];
    momentumy[i] = (1.0-lrate)*momentumy[i] + lrate*my[i];

  }

}

void World::cascade(vec2 pos){

  float* h = World::heightmap;

  ivec2 ipos = pos;
  int ind = ipos.x * dim.y + ipos.y;

  static const ivec2 n[] = {
    ivec2(-1, -1),
    ivec2(-1,  0),
    ivec2(-1,  1),
    ivec2( 0, -1),
    ivec2( 0,  1),
    ivec2( 1, -1),
    ivec2( 1,  0),
    ivec2( 1,  1)
  };

  //No Out-Of-Bounds

  struct Point {
    ivec2 pos;
    double h;
  };
  Point sn[8];
  int num = 0;
  for(auto& nn: n){
    ivec2 npos = ipos + nn;
    if(npos.x >=dim.x || npos.y >=dim.y
       || npos.x < 0 || npos.y < 0) continue;
    sn[num++] = { npos, h[npos.x * dim.y + npos.y] };
  }

  sort(std::begin(sn), std::begin(sn) + num, [&](const Point& a, const Point& b){
    return a.h > b.h;
  });

  const float maxdiff = 0.01f;
  const float settling = 0.5f;

  //Iterate over all Neighbors
  for (int i = 0; i < num; ++i) {

    auto& npos = sn[i].pos;

    int nind = npos.x * dim.y + npos.y;

    if(npos.x >= dim.x || npos.y >= dim.y
       || npos.x < 0 || npos.y < 0) continue;

    //Full Height-Different Between Positions!
    float diff = (h[ind] - h[nind]);
    if(diff == 0)   //No Height Difference
      continue;

    //The Amount of Excess Difference!
    float excess = abs(diff) - maxdiff;
    if(excess <= 0)  //No Excess
      continue;

    //Actual Amount Transferred
    float transfer = settling * excess / 2.0f;

    //Cap by Maximum Transferrable Amount
    if(diff > 0){
      h[ind] -= transfer;
      h[nind] += transfer;
    }
    else{
      h[ind] += transfer;
      h[nind] -= transfer;
    }

  }

}

#endif
