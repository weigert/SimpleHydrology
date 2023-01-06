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

  bool descend(float* track, float* mx, float* my, float scale);

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

bool Drop::descend(float* track, float* mx, float* my, float scale){

  // Pointers to Relevant Storage Buffers, Parameters

  glm::ivec2 dim = World::dim;
  glm::vec3 n = World::normal(pos);

  static float* h = World::heightmap;
  static float* p = World::discharge;
  static float* pd = Vegetation::density;

  //Initial Position

  glm::ivec2 ipos = pos;
  int ind = math::flatten(ipos, dim);

  // Termination Checks

  if(age > maxAge){
    World::height(ipos) += sediment;
    return false;
  }

  if(volume < minVol){
    World::height(ipos) += sediment;
    return false;
  }

  // Effective Parameter Set

  float effD = depositionRate*1.0-pd[ind];//max(0.0, );
  if(effD < 0) effD = 0;

  // Apply Forces to Particle

  // Gravity Force

  speed += gravity*vec2(n.x, n.z)/volume;

  // Momentum Transfer Force

  vec2 fspeed = vec2(World::momentumx[ind], World::momentumy[ind]);
  if(length(fspeed) > 0 && length(speed) > 0)
    speed += momentumTransfer*dot(normalize(fspeed), normalize(speed))/(volume + p[ind])*fspeed;

  // Dynamic Time-Step, Update

  if(length(speed) > 0)
    speed = sqrt(2.0f)*normalize(speed);

  pos   += speed;

  // Update Discharge, Momentum Tracking Maps

  track[ind] += volume;
  mx[ind] += volume*speed.x;
  my[ind] += volume*speed.y;

  //Out-Of-Bounds
  float h2;
  if(World::oob(pos))
    h2 = World::height(ipos)-0.003;
  else
    h2 = World::height(pos);

  //Mass-Transfer (in MASS)
  float c_eq = (1.0f+entrainment*World::getDischarge(ipos))*(World::height(ipos)-h2);
  if(c_eq < 0) c_eq = 0;
  float cdiff = c_eq - sediment;

  sediment += effD*cdiff;
  World::height(ipos) -= effD*cdiff;

  //Evaporate (Mass Conservative)
  sediment /= (1.0-evapRate);
  volume *= (1.0-evapRate);

  //Out-Of-Bounds
  if(World::oob(pos)){
    volume = 0.0;
    return false;
  }

  World::cascade(pos);

  age++;
  return true;

}

#endif
