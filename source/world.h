#ifndef SIMPLEHYDROLOGY_WORLD
#define SIMPLEHYDROLOGY_WORLD

#include "include/FastNoiseLite.h"
#include "include/math.h"

/*
SimpleHydrology - world.h

Defines our main storage buffers,
world updating functions for erosion
and vegetation.
*/

class World {

public:

  static unsigned int SEED;
  static glm::ivec2 dim;                      //Size of the Map

  // Storage Arrays

  static float heightmap[WSIZE*WSIZE];        //Flat Array
  static float discharge[WSIZE*WSIZE];        //Discharge Storage (Rivers)
  static float momentumx[WSIZE*WSIZE];        //Momentum X Storage (Rivers)
  static float momentumy[WSIZE*WSIZE];        //Momentum Y Storage (Rivers)

  // Parameters

  static float lrate;
  static float dischargeThresh;
  static float maxdiff;
  static float settling;

  // Main Update Methods

  static void generate();                     // Initialize Heightmap
  static bool oob(ivec2 pos);                 // Check Out-Of-Bounds

  static float& height(vec2 pos);             // Get Surface Height (Reference)
  static float getDischarge(vec2 pos);

  static glm::vec3 normal(vec2 pos);          // Compute Surface Normal
  static void erode(int cycles);              // Erosion Update Step
  static void cascade(vec2 pos);              // Perform Sediment Cascade


};

unsigned int World::SEED = 1;
glm::ivec2 World::dim = glm::vec2(WSIZE, WSIZE);

float World::heightmap[WSIZE*WSIZE] = {0.0};    //Flat Array
float World::discharge[WSIZE*WSIZE] = {0.0};    //Water Path Storage (Rivers)
float World::momentumx[WSIZE*WSIZE] = {0.0};    //Momentum X Storage (Rivers)
float World::momentumy[WSIZE*WSIZE] = {0.0};    //Momentum Y Storage (Rivers)

float World::lrate = 0.1f;
float World::dischargeThresh = 0.4f;
float World::maxdiff = 0.01f;
float World::settling = 0.8f;

#include "vegetation.h"
#include "water.h"

/*
================================================================================
                        World Method Implementations
================================================================================
*/

inline bool World::oob(ivec2 pos){
  if(pos.x >= dim.x) return true;
  if(pos.y >= dim.y) return true;
  if(pos.x < 0) return true;
  if(pos.y < 0) return true;
  return false;
}

inline float& World::height(vec2 pos){
  return heightmap[math::flatten(pos, dim)];
}

inline float World::getDischarge(vec2 pos){
  return erf(dischargeThresh*discharge[math::flatten(pos, dim)]);
}

glm::vec3 World::normal(vec2 pos){

  glm::vec3 n = glm::vec3(0);

  //Two large triangels adjacent to the plane (+Y -> +X) (-Y -> -X)
  n += glm::cross(glm::vec3( 0.0, SCALE*(height(pos+glm::vec2(0,1))-height(pos)), 1.0), glm::vec3( 1.0, SCALE*(height(pos+glm::vec2(1,0))-height(pos)), 0.0));
  n += glm::cross(glm::vec3( 0.0, SCALE*(height(pos-glm::vec2(0,1))-height(pos)),-1.0), glm::vec3(-1.0, SCALE*(height(pos-glm::vec2(1,0))-height(pos)), 0.0));

  //Two Alternative Planes (+X -> -Y) (-X -> +Y)
  n += glm::cross(glm::vec3( 1.0, SCALE*(height(pos+glm::vec2(1,0))-height(pos)), 0.0), glm::vec3( 0.0, SCALE*(height(pos-glm::vec2(0,1))-height(pos)),-1.0));
  n += glm::cross(glm::vec3(-1.0, SCALE*(height(pos-glm::vec2(1,0))-height(pos)), 0.0), glm::vec3( 0.0, SCALE*(height(pos+glm::vec2(0,1))-height(pos)), 1.0));

  return glm::normalize(n);

}

void World::generate(){

  std::cout<<"Generating New World"<<std::endl;
  std::cout<<"Seed: "<<SEED<<std::endl;

  std::cout<<"... generating height ..."<<std::endl;

  static FastNoiseLite noise; //Noise System
  float octaves = 8.0f;       //
  float lacunarity = 2.0f;    //
  float gain = 0.6f;          //
  float frequency = 1.0f;     //

  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFractalOctaves(octaves);
  noise.SetFractalLacunarity(lacunarity);
  noise.SetFractalGain(gain);
  noise.SetFrequency(frequency);

  float min, max = 0.0;
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = noise.GetNoise((i/dim.y)*(1.0/dim.x), (i%dim.y)*(1.0/dim.y), (double)(SEED%10000));
    if(heightmap[i] > max) max = heightmap[i];
    if(heightmap[i] < min) min = heightmap[i];
  }

  //Normalize
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = (heightmap[i] - min)/(max - min);
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

  //Update Fields
  for(int i = 0; i < dim.x*dim.y; i++){

    discharge[i] = (1.0-lrate)*discharge[i] + lrate*track[i];
    momentumx[i] = (1.0-lrate)*momentumx[i] + lrate*mx[i];
    momentumy[i] = (1.0-lrate)*momentumy[i] + lrate*my[i];

  }

}

void World::cascade(vec2 pos){

  // Get Non-Out-of-Bounds Neighbors

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

  struct Point {
    ivec2 pos;
    float h;
  };

  static Point sn[8];
  int num = 0;

  ivec2 ipos = pos;

  for(auto& nn: n){

    ivec2 npos = ipos + nn;

    if(World::oob(npos))
      continue;

    sn[num++] = { npos, World::height(npos) };

  }

  //Iterate over all sorted Neighbors

  sort(std::begin(sn), std::begin(sn) + num, [&](const Point& a, const Point& b){
    return a.h < b.h;
  });

  for (int i = 0; i < num; ++i) {

    auto& npos = sn[i].pos;

    //Full Height-Different Between Positions!
    float diff = (World::height(ipos) - World::height(npos));
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
      World::height(ipos) -= transfer;
      World::height(npos) += transfer;
    }
    else{
      World::height(ipos) += transfer;
      World::height(npos) -= transfer;
    }

  }

}

#endif
