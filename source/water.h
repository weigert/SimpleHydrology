#ifndef SIMPLEHYDROLOGY_WATER
#define SIMPLEHYDROLOGY_WATER

/*
SimpleHydrology - water.h

Defines our particle struct and
the method for descending / eroding
the landscape.
*/

struct Drop {

  Drop(glm::vec2 _pos){ pos = _pos; }   // Construct at Position

  // Properties

  int age = 0;
  glm::vec2 pos;
  glm::vec2 speed = glm::vec2(0.0);

  float volume = 1.0;                   // Droplet Water Volume
  float sediment = 0.0;                 // Droplet Sediment Concentration

  //Parameters

  static float maxAge;                  // Maximum Droplet Age
  static float minVol;                  // Minimum Droplet Volume
  static float evapRate;                // Droplet Evaporation Rate
  static float depositionRate;          // Droplet Deposition Rate
  static float entrainment;             // Additional Sediment per Discharge
  static float gravity;                 // Gravity Force Scale
  static float momentumTransfer;        // Rate of Momentum Transfer

  // Main Methods

  bool descend(float scale);

};

// Parameter Definitions

float Drop::evapRate = 0.001;
float Drop::depositionRate = 0.1;
float Drop::minVol = 0.01;
float Drop::maxAge = 500;

float Drop::entrainment = 10.0f;
float Drop::gravity = 1.0f;
float Drop::momentumTransfer = 1.0f;

/*
================================================================================
                        Drop Method Implementations
================================================================================
*/

bool Drop::descend(float scale){

  // Pointers to Relevant Storage Buffers, Parameters

  glm::ivec2 dim = World::dim;
  glm::vec3 n = reduce::normal(World::map, pos);

  //Initial Position

  glm::ivec2 ipos = pos;
  int ind = math::flatten(ipos, dim);

  // Termination Checks

  if(age > maxAge){
    World::map.get(ipos)->height += sediment;
    return false;
  }

  if(volume < minVol){
    World::map.get(ipos)->height += sediment;
    return false;
  }

  // Effective Parameter Set

  float effD = depositionRate;//*1.0-pd[ind];//max(0.0, );
  if(effD < 0) effD = 0;

  // Apply Forces to Particle

  // Gravity Force

  speed += gravity*vec2(n.x, n.z)/volume;

  // Momentum Transfer Force

  vec2 fspeed = vec2(World::map.get(ipos)->momentumx, World::map.get(ipos)->momentumy);
  if(length(fspeed) > 0 && length(speed) > 0)
    speed += momentumTransfer*dot(normalize(fspeed), normalize(speed))/(volume + World::map.get(ipos)->discharge)*fspeed;

  // Dynamic Time-Step, Update

  if(length(speed) > 0)
    speed = sqrt(2.0f)*normalize(speed);

  pos   += speed;

  // Update Discharge, Momentum Tracking Maps

  World::map.get(ipos)->discharge_track += volume;
  World::map.get(ipos)->momentumx_track += volume*speed.x;
  World::map.get(ipos)->momentumy_track += volume*speed.y;

//  track[ind] += volume;
//  mx[ind] += volume*speed.x;
//  my[ind] += volume*speed.y;

  //Out-Of-Bounds
  float h2;
  if(World::map.oob(pos))
    h2 = reduce::height(World::map, ipos)-0.003;
  else
    h2 = reduce::height(World::map, pos);

  //Mass-Transfer (in MASS)
  float c_eq = (1.0f+entrainment*reduce::discharge(World::map, ipos))*(height(World::map, ipos)-h2);
  if(c_eq < 0) c_eq = 0;
  float cdiff = c_eq - sediment;

  sediment += effD*cdiff;
  World::map.get(ipos)->height -= effD*cdiff;

  //Evaporate (Mass Conservative)
  sediment /= (1.0-evapRate);
  volume *= (1.0-evapRate);

  //Out-Of-Bounds
  if(World::map.oob(pos)){
    volume = 0.0;
    return false;
  }

  World::cascade(pos);

  age++;
  return true;

}

#endif
