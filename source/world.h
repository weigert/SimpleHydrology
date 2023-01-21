#ifndef SIMPLEHYDROLOGY_WORLD
#define SIMPLEHYDROLOGY_WORLD

#include "include/FastNoiseLite.h"
#include "include/math.h"

#include "cellpool.h"

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

  static quadmap::map<reduce::cell> map;

  // Parameters

  static float lrate;
  static float dischargeThresh;
  static float maxdiff;
  static float settling;

  // Main Update Methods

  static void generate();                     // Initialize Heightmap

  static void erode(int cycles);              // Erosion Update Step
  static void cascade(vec2 pos);              // Perform Sediment Cascade

};

unsigned int World::SEED = 1;
glm::ivec2 World::dim = glm::vec2(WSIZE, WSIZE);

quadmap::map<reduce::cell> World::map;

float World::lrate = 0.1f;
float World::maxdiff = 0.01f;
float World::settling = 0.8f;

#include "vegetation.h"
#include "water.h"

/*
================================================================================
                        World Method Implementations
================================================================================
*/

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

  for(auto& index: map.indices){

      for(int i = index.pos.x; i < index.pos.x + index.res.x; i++)
      for(int j = index.pos.y; j < index.pos.y + index.res.y; j++){
        index.get(ivec2(i, j))->height = noise.GetNoise((double)i/(double)dim.x, (double)j/(double)dim.y, (double)(SEED%10000));
        if(index.get(ivec2(i, j))->height > max) max = index.get(ivec2(i, j))->height;
        if(index.get(ivec2(i, j))->height < min) min = index.get(ivec2(i, j))->height;
      }

  }

  //Normalize
  for(auto& index: map.indices){

    for(int i = index.pos.x; i < index.pos.x + index.res.x; i++)
    for(int j = index.pos.y; j < index.pos.y + index.res.y; j++){
      index.get(ivec2(i, j))->height = (index.get(ivec2(i, j))->height - min)/(max - min);
    }

  }

}

/*
===================================================
          HYDRAULIC EROSION FUNCTIONS
===================================================
*/
void World::erode(int cycles){

  for(auto& index: map.indices){

    for(int i = index.pos.x; i < index.pos.x + index.res.x; i++)
    for(int j = index.pos.y; j < index.pos.y + index.res.y; j++){
      index.get(ivec2(i, j))->discharge_track = 0;
      index.get(ivec2(i, j))->momentumx_track = 0;
      index.get(ivec2(i, j))->momentumy_track = 0;
    }

  }

  //Do a series of iterations!
  for(auto& index: map.indices)
  for(int i = 0; i < cycles; i++){

    //Spawn New Particle

    glm::vec2 newpos = index.pos + ivec2(rand()%index.res.x, rand()%index.res.y);
    Drop drop(newpos);

    while(drop.descend(SCALE));

  }

  //Update Fields
  for(auto& index: map.indices){

    for(int i = index.pos.x; i < index.pos.x + index.res.x; i++)
    for(int j = index.pos.y; j < index.pos.y + index.res.y; j++){

      index.get(ivec2(i, j))->discharge = (1.0-lrate)*index.get(ivec2(i, j))->discharge + lrate*index.get(ivec2(i, j))->discharge_track;//track[math::flatten(ivec2(i, j), World::dim)];
      index.get(ivec2(i, j))->momentumx = (1.0-lrate)*index.get(ivec2(i, j))->momentumx + lrate*index.get(ivec2(i, j))->momentumx_track;//mx[math::flatten(ivec2(i, j), World::dim)];
      index.get(ivec2(i, j))->momentumy = (1.0-lrate)*index.get(ivec2(i, j))->momentumy + lrate*index.get(ivec2(i, j))->momentumy_track;//my[math::flatten(ivec2(i, j), World::dim)];

    }
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

    if(World::map.oob(npos))
      continue;

    sn[num++] = { npos, World::map.get(npos)->height };

  }

  //Iterate over all sorted Neighbors

  sort(std::begin(sn), std::begin(sn) + num, [&](const Point& a, const Point& b){
    return a.h < b.h;
  });

  for (int i = 0; i < num; ++i) {

    auto& npos = sn[i].pos;

    //Full Height-Different Between Positions!
    float diff = (World::map.get(ipos)->height - World::map.get(npos)->height);
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
      World::map.get(ipos)->height -= transfer;
      World::map.get(npos)->height += transfer;
    }
    else{
      World::map.get(ipos)->height += transfer;
      World::map.get(npos)->height -= transfer;
    }

  }

}

#endif
