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
===================================================
          HYDRAULIC EROSION FUNCTIONS
===================================================
*/
void World::erode(int cycles){

  for(auto& node: map.nodes)
  for(auto [cell, pos]: node.s){
    cell.discharge_track = 0;
    cell.momentumx_track = 0;
    cell.momentumy_track = 0;
  }

  //Do a series of iterations!
  for(auto& node: map.nodes)
  for(int i = 0; i < cycles; i++){

    //Spawn New Particle

    glm::vec2 newpos = node.pos + ivec2(rand()%quad::tileres.x, rand()%quad::tileres.y);
    Drop drop(newpos);

    while(drop.descend());

  }

  //Update Fields
  const float l = lrate/float(quad::lodarea);
  for(auto& node: map.nodes)
  for(auto [cell, pos]: node.s){
    cell.discharge = (1.0f-l)*cell.discharge + l*cell.discharge_track;
    cell.momentumx = (1.0f-l)*cell.momentumx + l*cell.momentumx_track;
    cell.momentumy = (1.0f-l)*cell.momentumy + l*cell.momentumy_track;
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

    ivec2 npos = ipos + quad::lodsize*nn;

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
    float excess = abs(diff) - maxdiff * quad::lodsize;
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
