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

  const glm::ivec2 ipos = pos;

  quadmap::node<reduce::cell>* node = World::map.get(ipos);
  if(node == NULL)
    return false;

  reduce::cell* cell = node->get(ipos);
  if(cell == NULL)
    return false;

  const glm::vec3 n = reduce::normal(node, ipos);

  // Termination Checks

  if(age > maxAge){
    cell->height += sediment;
    return false;
  }

  if(volume < minVol){
    cell->height += sediment;
    return false;
  }

  // Effective Parameter Set

  float effD = depositionRate;
  if(effD < 0) effD = 0;

  // Apply Forces to Particle

  // Gravity Force

  speed += RES*gravity*vec2(n.x, n.z)/volume;///float(node->s.scale/node->s.scale;

  // Momentum Transfer Force

  vec2 fspeed = vec2(cell->momentumx, cell->momentumy);
  if(length(fspeed) > 0 && length(speed) > 0)
    speed += RES*momentumTransfer*dot(normalize(fspeed), normalize(speed))/(volume + cell->discharge)*fspeed;

  // Dynamic Time-Step, Update

  if(length(speed) > 0)
    speed = (node->s.scale*sqrt(2.0f))*normalize(speed);

  pos   += speed;

  // Update Discharge, Momentum Tracking Maps

  cell->discharge_track += volume/float(RES);///(node->s.scale*node->s.scale);
  cell->momentumx_track += volume*speed.x/float(RES);///(node->s.scale*node->s.scale);
  cell->momentumy_track += volume*speed.y/float(RES);///(node->s.scale*node->s.scale);

  //Out-Of-Bounds
  float h2;
  if(node->oob(pos))
    h2 = cell->height-0.003;
  else
    h2 = node->get(pos)->height;

  //Mass-Transfer (in MASS)
  float c_eq = (1.0f+entrainment*reduce::discharge(node, ipos))*(cell->height-h2);
  if(c_eq < 0) c_eq = 0;
  float cdiff = (c_eq - sediment)/(float(RES*RES));

  sediment += effD*cdiff;
  cell->height -= effD*cdiff;

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
