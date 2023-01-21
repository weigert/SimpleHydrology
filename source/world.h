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
  static quad::map map;

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

quad::map World::map;

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

  for(auto& node: map.nodes){

      // Highest Res
      for(int i = 0; i < node.res.x; i++)
      for(int j = 0; j < node.res.y; j++){
        vec2 p = (vec2(node.pos) + vec2(i, j))/vec2(node.res);
        node.get(node.pos + ivec2(i, j))->height = noise.GetNoise(p.x, p.y, (float)(SEED%10000));
      }

  }

}

/*
===================================================
          HYDRAULIC EROSION FUNCTIONS
===================================================
*/
void World::erode(int cycles){

  for(auto& node: map.nodes){

    for(int i = node.pos.x; i < node.pos.x + node.res.x; i++)
    for(int j = node.pos.y; j < node.pos.y + node.res.y; j++){
      node.get(ivec2(i, j))->discharge_track = 0;
      node.get(ivec2(i, j))->momentumx_track = 0;
      node.get(ivec2(i, j))->momentumy_track = 0;
    }

  }

  //Do a series of iterations!
  for(auto& node: map.nodes)
  for(int i = 0; i < cycles; i++){

    //Spawn New Particle

    glm::vec2 newpos = node.pos + ivec2(rand()%node.res.x, rand()%node.res.y);
    Drop drop(newpos);

    while(drop.descend(SCALE));

  }

  float l = lrate;///RES/RES;

  //Update Fields
  for(auto& node: map.nodes){

    for(int i = node.pos.x; i < node.pos.x + node.res.x; i++)
    for(int j = node.pos.y; j < node.pos.y + node.res.y; j++){

      node.get(ivec2(i, j))->discharge = (1.0-l)*node.get(ivec2(i, j))->discharge + l*node.get(ivec2(i, j))->discharge_track;//track[math::flatten(ivec2(i, j), World::dim)];
      node.get(ivec2(i, j))->momentumx = (1.0-l)*node.get(ivec2(i, j))->momentumx + l*node.get(ivec2(i, j))->momentumx_track;//mx[math::flatten(ivec2(i, j), World::dim)];
      node.get(ivec2(i, j))->momentumy = (1.0-l)*node.get(ivec2(i, j))->momentumy + l*node.get(ivec2(i, j))->momentumy_track;//my[math::flatten(ivec2(i, j), World::dim)];

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

    ivec2 npos = ipos + RES*nn;

    if(World::map.oob(npos))
      continue;

    sn[num++] = { npos, World::map.get(npos)->get(npos)->height };

  }

  //Iterate over all sorted Neighbors

  sort(std::begin(sn), std::begin(sn) + num, [&](const Point& a, const Point& b){
    return a.h < b.h;
  });

  for (int i = 0; i < num; ++i) {

    auto& npos = sn[i].pos;

    //Full Height-Different Between Positions!
    float diff = World::map.get(ipos)->get(ipos)->height - World::map.get(npos)->get(npos)->height;
    if(diff == 0)   //No Height Difference
      continue;

    //The Amount of Excess Difference!
    float excess = abs(diff) - RES*maxdiff;
    if(excess <= 0)  //No Excess
      continue;

    //Actual Amount Transferred
    float transfer = settling * excess / 2.0f;

    //Cap by Maximum Transferrable Amount
    if(diff > 0){
      World::map.get(ipos)->get(ipos)->height -= transfer;
      World::map.get(npos)->get(npos)->height += transfer;
    }
    else{
      World::map.get(ipos)->get(ipos)->height += transfer;
      World::map.get(npos)->get(npos)->height -= transfer;
    }

  }

}

#endif
