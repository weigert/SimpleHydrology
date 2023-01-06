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

/*
  New World Data-Storage Data Structure:

  Either we do sort objects using an interleaved or a non-interleaved property buffer.
  Interleaved is of course easier.

  Then I have a set of map-sections, which index the pool.

  A droplet then operates in a specific map-section, which later can contain a stride.
  The world then simply represents the quadtree management interface for accessing
  various areas of the map.

  Once I can create a basic map, I need to render it.
  Then the erosion should also work directly.

  1. Mapcell contains base Structure
  2. A cellpool lets me return a cell pool indexing struct,
    which is the base element used by the quadtree
    That's basically it.
*/

// Position Storage Buffer

struct MapCell {

  float height;
  float discharge;
  float momentumx;
  float momentumy;

};

template<typename T>
struct MapSection {

  T* start;
  size_t size;

};

template<typename T>
struct CellPool {

  T* start = NULL;
  size_t size = 0;

  deque<MapSection<T>> free;

  CellPool(){}
  CellPool(size_t _size){
    reserve(_size);
  }

  ~CellPool(){
    if(start != NULL)
      delete[] start;
  }

  void reserve(size_t _size){
    size = _size;
    start = new T[size];
    free.emplace_front(start, size);
  }

  MapSection<T> get(size_t _size){

    if(free.empty())
      return {NULL, 0};

    if(_size > size)
      return {NULL, 0};

    if(free.front().size < _size)
      return {NULL, 0};

    return free.front();

  }

};

CellPool<MapCell> pool;





class World {

public:

  static unsigned int SEED;
  static glm::ivec2 dim;                      //Size of the Map

  // Storage Arrays

  static MapSection<MapCell> section;

  // Parameters

  static float lrate;
  static float dischargeThresh;
  static float maxdiff;
  static float settling;

  // Main Update Methods

  static void generate();                     // Initialize Heightmap
  static bool oob(ivec2 pos);                 // Check Out-Of-Bounds

  static MapCell& get(vec2 pos);              // Get Surface Height (Reference)
  static float getDischarge(vec2 pos);

  static glm::vec3 normal(vec2 pos);          // Compute Surface Normal
  static void erode(int cycles);              // Erosion Update Step
  static void cascade(vec2 pos);              // Perform Sediment Cascade


};

unsigned int World::SEED = 1;
glm::ivec2 World::dim = glm::vec2(WSIZE, WSIZE);

MapSection<MapCell> World::section;

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

inline MapCell& World::get(vec2 pos){
  return *(section.start + math::flatten(pos, dim));
}

inline float World::getDischarge(vec2 pos){
  return erf(dischargeThresh*World::get(pos).discharge);
}

glm::vec3 World::normal(vec2 pos){

  glm::vec3 n = glm::vec3(0);

  //Two large triangels adjacent to the plane (+Y -> +X) (-Y -> -X)
  if(!World::oob(pos+glm::vec2( 1, 1)))
    n += glm::cross(glm::vec3( 0.0, SCALE*(get(pos+glm::vec2(0,1)).height - get(pos).height), 1.0), glm::vec3( 1.0, SCALE*(get(pos+glm::vec2(1,0)).height - get(pos).height), 0.0));

  if(!World::oob(pos+glm::vec2(-1,-1)))
    n += glm::cross(glm::vec3( 0.0, SCALE*(get(pos-glm::vec2(0,1)).height - get(pos).height),-1.0), glm::vec3(-1.0, SCALE*(get(pos-glm::vec2(1,0)).height - get(pos).height), 0.0));

  //Two Alternative Planes (+X -> -Y) (-X -> +Y)
  if(!World::oob(pos+glm::vec2( 1,-1)))
    n += glm::cross(glm::vec3( 1.0, SCALE*(get(pos+glm::vec2(1,0)).height - get(pos).height), 0.0), glm::vec3( 0.0, SCALE*(get(pos-glm::vec2(0,1)).height - get(pos).height),-1.0));

  if(!World::oob(pos+glm::vec2(-1, 1)))
    n += glm::cross(glm::vec3(-1.0, SCALE*(get(pos-glm::vec2(1,0)).height - get(pos).height), 0.0), glm::vec3( 0.0, SCALE*(get(pos+glm::vec2(0,1)).height - get(pos).height), 1.0));

  return glm::normalize(n);

}

void World::generate(){

  std::cout<<"Generating New World"<<std::endl;
  std::cout<<"Seed: "<<SEED<<std::endl;

  pool.reserve(WSIZE*WSIZE);
  World::section = pool.get(WSIZE*WSIZE);

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
  for(int i = 0; i < dim.x; i++)
  for(int j = 0; j < dim.y; j++){
    World::get(ivec2(i, j)).height = noise.GetNoise((double)i/(double)dim.x, (double)j/(double)dim.y, (double)(SEED%10000));
    if(World::get(ivec2(i, j)).height > max) max = World::get(ivec2(i, j)).height;
    if(World::get(ivec2(i, j)).height < min) min = World::get(ivec2(i, j)).height;
  }

  //Normalize
  for(int i = 0; i < dim.x; i++)
  for(int j = 0; j < dim.y; j++){
    World::get(ivec2(i, j)).height = (World::get(ivec2(i, j)).height - min)/(max - min);
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
  for(int i = 0; i < dim.x; i++)
  for(int j = 0; j < dim.y; j++){

    World::get(ivec2(i, j)).discharge = (1.0-lrate)*World::get(ivec2(i, j)).discharge + lrate*track[math::flatten(ivec2(i, j), World::dim)];
    World::get(ivec2(i, j)).momentumx = (1.0-lrate)*World::get(ivec2(i, j)).momentumx + lrate*mx[math::flatten(ivec2(i, j), World::dim)];
    World::get(ivec2(i, j)).momentumy = (1.0-lrate)*World::get(ivec2(i, j)).momentumy + lrate*my[math::flatten(ivec2(i, j), World::dim)];

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

    sn[num++] = { npos, World::get(npos).height };

  }

  //Iterate over all sorted Neighbors

  sort(std::begin(sn), std::begin(sn) + num, [&](const Point& a, const Point& b){
    return a.h < b.h;
  });

  for (int i = 0; i < num; ++i) {

    auto& npos = sn[i].pos;

    //Full Height-Different Between Positions!
    float diff = (World::get(ipos).height - World::get(npos).height);
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
      World::get(ipos).height -= transfer;
      World::get(npos).height += transfer;
    }
    else{
      World::get(ipos).height += transfer;
      World::get(npos).height -= transfer;
    }

  }

}

#endif
