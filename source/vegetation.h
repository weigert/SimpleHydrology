#ifndef SIMPLEHYDROLOGY_VEGETATION
#define SIMPLEHYDROLOGY_VEGETATION

/*
SimpleHydrology - vegatation.h

Defines our vegetation / plant
particles and their static Density
maps and update functions.
*/

struct Plant {

  Plant(vec2 _pos){ pos = _pos; };

  // Properties

  glm::vec2 pos;
  float size = 0.0;

  // Parameters

  static float maxSize;
  static float growRate;
  static float maxSteep;
  static float maxDischarge;

  // Update Functions

  void root(float* density, glm::ivec2 dim, float factor);
  void grow();
  static bool spawn(vec2 pos);
  bool die();

};

float Plant::maxSize = 1.0f;
float Plant::growRate = 0.05f;
float Plant::maxSteep = 0.8f;
float Plant::maxDischarge = 0.2f;

// Vegetation Struct (Plant Container)

struct Vegetation {

  static std::vector<Plant> plants;
  static float density[WSIZE*WSIZE];     //Density for Plants
  static bool grow();

};

std::vector<Plant> Vegetation::plants;
float Vegetation::density[WSIZE*WSIZE]{0.0f};

/*
================================================================================
                      Vegetation Method Implementations
================================================================================
*/

// Plant Specific Methods

void Plant::grow(){
  size += growRate*(maxSize-size);
};

bool Plant::die(){

  if( World::map.discharge(pos) >= Plant::maxDischarge ) return true;
  if( rand()%1000 == 0 ) return true;
  return false;

}

bool Plant::spawn( vec2 pos ){

  if( World::map.discharge(pos) >= Plant::maxDischarge ) return false;
  glm::vec3 n = reduce::normal(World::map, pos);
  if( n.y < Plant::maxSteep ) return false;

  return true;

}

void Plant::root(float* density, glm::ivec2 dim, float f){

  int ind = math::flatten(pos, dim);

  //Can always do this one
  density[ind]       += f*1.0;

  if(pos.x > 0){
    //
    density[ind - WSIZE] += f*0.6;      //(-1, 0)

    if(pos.y > 0)
      density[ind - WSIZE-1] += f*0.4;    //(-1, -1)

    if(pos.y < WSIZE-1)
      density[ind - WSIZE+1] += f*0.4;    //(-1, 1)
  }

  if(pos.x < WSIZE-1){
    //
    density[ind + WSIZE] += f*0.6;    //(1, 0)

    if(pos.y > 0)
      density[ind + WSIZE-1] += f*0.4;    //(1, -1)

    if(pos.y < WSIZE-1)
      density[ind + WSIZE+1] += f*0.4;    //(1, 1)
  }

  if(pos.y > 0)
    density[ind - 1]   += f*0.6;    //(0, -1)

  if(pos.y < WSIZE-1)
    density[ind + 1]   += f*0.6;    //(0, 1)
}

// Vegetation Specific Methods

bool Vegetation::grow(){

  /*
  //Random Position
  {

    int x = rand()%(World::dim.x);
    int y = rand()%(World::dim.y);

    if( Plant::spawn(vec2(x, y)) ){

      plants.emplace_back(vec2(x, y));
      plants.back().root(density, World::dim, 1.0);

    }

  }

  // Iterate over Plants

  for(int i = 0; i < plants.size(); i++){

    //Grow the Plant

    plants[i].grow();

    // Check for Kill Plant

    if( plants[i].die() ){

       plants[i].root(density, World::dim, -1.0);
       plants.erase(plants.begin()+i);
       i--;
       continue;

    }

    // Check for Growth

    if(rand()%50 != 0)
      continue;

    //Find New Position
    glm::vec2 npos = plants[i].pos + glm::vec2(rand()%9-4, rand()%9-4);
    int nind = math::flatten(npos, World::dim);

    //Check for Out-Of-Bounds
    if(World::map.oob(npos))
      continue;

    if(reduce::discharge(World::map, npos) >= Plant::maxDischarge)
      continue;

    if((float)(rand()%1000)/1000.0 <= density[nind])
      continue;

    glm::vec3 n = reduce::normal(World::map, npos);

    if( n.y <= Plant::maxSteep )
      continue;

    plants.emplace_back(npos);
    plants.back().root(density, World::dim, 1.0);

  }

  */


  return true;

};

#endif
