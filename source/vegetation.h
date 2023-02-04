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

  void root(float factor);
  void grow();
  static bool spawn(vec2 pos);
  bool die();

};

float Plant::maxSize = 1.5f;
float Plant::growRate = 0.05f;
float Plant::maxSteep = 0.8f;
float Plant::maxDischarge = 0.3f;

// Vegetation Struct (Plant Container)

struct Vegetation {

  static std::vector<Plant> plants;
  static bool grow();

};

std::vector<Plant> Vegetation::plants;

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
  glm::vec3 n = World::map.normal(pos);
  if( n.y < Plant::maxSteep ) return false;

  return true;

}

void Plant::root(float f){

  quad::cell* c;

  c = World::map.getCell( pos + vec2( 0, 0) );
  if(c != NULL) c->rootdensity += f*1.0f;

  c = World::map.getCell( pos + vec2( 1, 0) );
  if(c != NULL) c->rootdensity += f*0.6f;

  c = World::map.getCell( pos + vec2(-1, 0) );
  if(c != NULL) c->rootdensity += f*0.6f;

  c = World::map.getCell( pos + vec2( 0, 1) );
  if(c != NULL) c->rootdensity += f*0.6f;

  c = World::map.getCell( pos + vec2( 0,-1) );
  if(c != NULL) c->rootdensity += f*0.6f;

  c = World::map.getCell( pos + vec2(-1,-1) );
  if(c != NULL) c->rootdensity += f*0.4f;

  c = World::map.getCell( pos + vec2( 1,-1) );
  if(c != NULL) c->rootdensity += f*0.4f;

  c = World::map.getCell( pos + vec2(-1, 1) );
  if(c != NULL) c->rootdensity += f*0.4f;

  c = World::map.getCell( pos + vec2( 1, 1) );
  if(c != NULL) c->rootdensity += f*0.4f;

}

// Vegetation Specific Methods

bool Vegetation::grow(){

  //Random Position
  {

    int x = rand()%(quad::res.x);
    int y = rand()%(quad::res.y);

    if( Plant::spawn(vec2(x, y)) ){

      plants.emplace_back(vec2(x, y));
      plants.back().root(1.0);

    }

  }

  // Iterate over Plants

  for(int i = 0; i < plants.size(); i++){

    //Grow the Plant

    plants[i].grow();

    // Check for Kill Plant

    if( plants[i].die() ){

       plants[i].root(-1.0);
       plants.erase(plants.begin()+i);
       i--;
       continue;

    }

    // Check for Growth

    if(rand()%20 != 0)
      continue;

    //Find New Position
    glm::vec2 npos = plants[i].pos + glm::vec2(rand()%9-4, rand()%9-4);

    //Check for Out-Of-Bounds
    if(World::map.oob(npos))
      continue;

    if(World::map.discharge(npos) >= Plant::maxDischarge)
      continue;

    if((float)(rand()%1000)/1000.0 <= World::map.getCell(npos)->rootdensity)
      continue;

    glm::vec3 n = World::map.normal(npos);

    if( n.y <= Plant::maxSteep )
      continue;

    plants.emplace_back(npos);
    plants.back().root(1.0);

  }

  return true;

};

#endif
