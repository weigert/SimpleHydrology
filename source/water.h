#ifndef SIMPLEHYDROLOGY_WATER
#define SIMPLEHYDROLOGY_WATER

/*
SimpleHydrology - water.h

Defines our particle struct and
the method for descending / eroding
the landscape.
*/

struct Drop{

  Drop(glm::vec2 _pos){ pos = _pos; }   // Construct at Position

  int age = 0;
  glm::vec2 pos;
  glm::vec2 speed = glm::vec2(0.0);

  float volume = 1.0;                   // Droplet Water Volume
  float sediment = 0.0;                 // Droplet Sediment Concentration

  //Parameters
  const float density = 1.0;  //This gives varying amounts of inertia and stuff...
  const float evapRate = 0.001;
  const float depositionRate = 1.2*0.08;
  const float minVol = 0.01;
  const float friction = 0.25;
  const float volumeFactor = 0.5; //"Water Deposition Rate"

  // Main Methods

  bool descend(float* track, float* mx, float* my, float scale);

};

bool Drop::descend(float* track, float* mx, float* my, float scale){

  static float* h = World::heightmap;
  static float* p = World::discharge;
  static float* pd = Vegetation::density;
  glm::ivec2 dim = World::dim;
  glm::vec3 n = World::normal((int)pos.x * dim.y + (int)pos.y);

  if(volume < minVol)
    return false;

  //Initial Position
  glm::ivec2 ipos = pos;
  int ind = ipos.x*dim.y+ipos.y;

  //Effective Parameter Set
  /* Higher plant density means less erosion */
  float effD = depositionRate*1.0-pd[ind];//max(0.0, );
  if(effD < 0) effD = 0;

  /* Higher Friction, Lower Evaporation in Streams
  makes particles prefer established streams -> "curvy" */

  float effF = friction*(1.0-erf(p[ind]));
  float effR = evapRate;//*(1.0-0.2*p[ind]);

  if(age > 500){
    h[ind] += sediment;
    return false;
  }

  //Particle is Not Accelerated
  //if(length(vec2(n.x, n.z))*effF < 1E-5)
  //  return false;


  float dt = 1.0f;
  if(length(speed) > 0)
    dt = sqrt(2)/length(speed);

  vec2 fspeed = vec2(World::momentumx[ind], World::momentumy[ind]);

  float transfer = 0.0f;
  if(length(fspeed) > 0 && length(speed) > 0)
    transfer = dot(normalize(fspeed), normalize(speed));

  speed += dt*2.0f*vec2(n.x, n.z)/volume;

  speed += transfer*dt/(volume + p[ind])*fspeed;

  if(length(speed) > 0)
    speed = sqrt(2.0f)*normalize(speed);

  pos   += speed;

  // Functioning Version
  track[ind] += dt*volume;
  mx[ind] += dt*volume*speed.x;
  my[ind] += dt*volume*speed.y;

//  vec2 tspeed = vec2(0.0f);
//  if(length(fspeed) > 0 && length(speed) > 0)
//    tspeed = normalize(mix(normalize(fspeed), normalize(speed), transfer));
//  else tspeed = speed;

  //New Position
  int nind = (int)pos.x*dim.y+(int)pos.y;

  //Out-Of-Bounds
  float h2;
  if(!glm::all(glm::greaterThanEqual(pos, glm::vec2(0))) ||
     !glm::all(glm::lessThan((glm::ivec2)pos, dim))){
       h2 = h[ind]-0.001;
   } else {
     h2 = h[nind];
   }

    //Mass-Transfer (in MASS)
    float c_eq = (1.0f+10*erf(0.2*p[ind]))*(h[ind]-h2);
    if(c_eq < 0) c_eq = 0;//max(0.0, (h[ind]-h[nind]));
    float cdiff = c_eq - sediment;

    sediment += dt*effD*cdiff;
    h[ind] -= dt*effD*cdiff;

    //Evaporate (Mass Conservative)
    sediment /= (1.0-effR);
    volume *= (1.0-effR);


  //Out-Of-Bounds
  if(!glm::all(glm::greaterThanEqual(pos, glm::vec2(0))) ||
     !glm::all(glm::lessThan((glm::ivec2)pos, dim))){
       volume = 0.0;
       return false;
   }

  World::cascade(pos);

  age++;
  return true;

}

#endif
