struct Drop{
  //Construct Particle at Position
  Drop(glm::vec2 _pos){ pos = _pos; }
  Drop(glm::vec2 _p, glm::ivec2 dim, float v){
    pos = _p;
    int index = _p .x*dim.y+_p.y;
    volume = v;
  }

  //Properties
  int age = 0;
  int index;
  glm::vec2 pos;
  glm::vec2 speed = glm::vec2(0.0);
  float volume = 1.0;   //This will vary in time
  float sediment = 0.0; //Sediment concentration

  //Parameters
  const float density = 1.0;  //This gives varying amounts of inertia and stuff...
  const float evapRate = 0.001;
  const float depositionRate = 1.2*0.08;
  const float minVol = 0.01;
  const float friction = 0.25;
  const float volumeFactor = 0.5; //"Water Deposition Rate"

  //Number of Spills Left
  int spill = 0;

  bool descend(glm::vec3 n, float* track, float* mx, float* my, glm::ivec2 dim, float scale);
  bool flood(float* h, float* pool, glm::ivec2 dim);

  static void cascade(vec2 pos, glm::ivec2 dim, float* h, float* p){

    ivec2 ipos = pos;
    int ind = ipos.x * dim.y + ipos.y;

    if(p[ind] > 0) return; //Don't do this with water

    //Neighbor Positions (8-Way)
    const int nx[8] = {-1,-1,-1, 0, 0, 1, 1, 1};
    const int ny[8] = {-1, 0, 1,-1, 1,-1, 0, 1};

    const float maxdiff = 0.01f;
    const float settling = 0.1f;

    //Iterate over all Neighbors
    for(int m = 0; m < 8; m++){

      ivec2 npos = ipos + ivec2(nx[m], ny[m]);
      int nind = npos.x * dim.y + npos.y;

      if(npos.x >= dim.x || npos.y >= dim.y
         || npos.x < 0 || npos.y < 0) continue;

      if(p[nind] > 0) continue; //Don't do this with water

      //Full Height-Different Between Positions!
      float diff = (h[ind] - h[nind]);
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
        h[ind] -= transfer;
        h[nind] += transfer;
      }
      else{
        h[ind] += transfer;
        h[nind] -= transfer;
      }

    }

  }

};

bool Drop::descend(glm::vec3 n, float* track, float* mx, float* my, glm::ivec2 dim, float scale){

  static float* h = World::heightmap;
  static float* p = World::waterpath;
  static float* b = World::waterpool;
  //static float* mx = World::momentumx;
  //static float* my = World::momentumy;
  static float* pd = World::plantdensity;

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


  if(age > 500)
    return false;

  //Particle is Not Accelerated
  //if(length(vec2(n.x, n.z))*effF < 1E-5)
  //  return false;

  // Functioning Version
  track[ind] += volume;
  mx[ind] += volume*speed.x;
  my[ind] += volume*speed.y;

  speed = mix(speed, vec2(n.x, n.z), n.y);

  vec2 fspeed = vec2(World::momentumx[ind], World::momentumy[ind]);
  if(length(fspeed) > 0 && p[ind] > 0)
    speed = mix(speed, fspeed/p[ind], p[ind]/(volume + p[ind]));

  speed = sqrt(2.0f)*normalize(speed);
  pos   += speed;

  //New Position
  int nind = (int)pos.x*dim.y+(int)pos.y;

  //Out-Of-Bounds
  if(!glm::all(glm::greaterThanEqual(pos, glm::vec2(0))) ||
     !glm::all(glm::lessThan((glm::ivec2)pos, dim))){
       volume = 0.0;
       return false;
   }

   //Particle is in Pool
   if(b[nind] > 0.0){
     return false;
   }

  //Mass-Transfer (in MASS)
  float c_eq = h[ind]-h[nind];
  if(c_eq < 0) c_eq = 0;//max(0.0, (h[ind]-h[nind]));
  float cdiff = c_eq - sediment;
  sediment += effD*cdiff;
  h[ind] -= effD*cdiff;

  //Evaporate (Mass Conservative)
  sediment /= (1.0-effR);
  volume *= (1.0-effR);

  cascade(pos, dim, h, b);

  age++;
  return true;

}

#include <unordered_map>

/*

Flooding Algorithm Overhaul:
  Currently, I can only flood at my position as long as we are rising.
  Then I return and let the particle descend. This should only happen if I can't find a closed set to fill.

  So: Rise and fill, removing the volume as we go along.
  Then: If we find a lower point, try to rise and fill from there.

*/

bool Drop::flood(float* h, float* p, glm::ivec2 dim){
using namespace glm;

  if(volume < minVol || spill-- <= 0)
    return false;

  //Either try to find a closed set under this plane, which has a certain volume,
  //or raise the plane till we find the correct closed set height.
  //And only if it can't be found, re-emit the particle.

  bool tried[dim.x*dim.y] = {false};

  unordered_map<int, float> boundary;
  vector<ivec2> floodset;

  bool drainfound = false;
  ivec2 drain;

  //Returns whether the set is closed at given height

  const function<bool(ivec2, float)> findset = [&](ivec2 i, float plane){

    if(i.x < 0 || i.y < 0 || i.x >= dim.x || i.y >= dim.y)
      return true;

    int ind = i.x*dim.y+i.y;

    if(tried[ind]) return true;
    tried[ind] = true;

    //Wall / Boundary
    if((h[ind] + p[ind]) > plane){
      boundary[ind] = h[ind] + p[ind];
      return true;
    }

    //Drainage Point
    if((h[ind] + p[ind]) < plane){

      //No Drain yet
      if(!drainfound)
        drain = i;

      //Lower Drain
      else if(p[ind] + h[ind] < p[drain.x*dim.y+drain.y] + h[drain.x*dim.y*drain.y])
        drain = i;

      drainfound = true;
      return false;

    }

    floodset.push_back(i);

    if(!findset(i+ivec2( 1, 0), plane)) return false;
    if(!findset(i-ivec2( 1, 0), plane)) return false;
    if(!findset(i+ivec2( 0, 1), plane)) return false;
    if(!findset(i-ivec2( 0, 1), plane)) return false;
    if(!findset(i+ivec2( 1, 1), plane)) return false;
    if(!findset(i-ivec2( 1, 1), plane)) return false;
    if(!findset(i+ivec2(-1, 1), plane)) return false;
    if(!findset(i-ivec2(-1, 1), plane)) return false;

    return true;

  };

  ivec2 ipos = pos;
  int ind = ipos.x*dim.y+ipos.y;
  float plane = h[ind] + p[ind];

  pair<int, float> minbound = pair<int, float>(ind, plane);

  while(volume > minVol && findset(ipos, plane)){

    //Find the Lowest Element on the Boundary
    minbound = (*boundary.begin());
    for(auto& b : boundary)
    if(b.second < minbound.second)
      minbound = b;

    //Compute the Height of our Volume over the Set
    float vheight = volume*volumeFactor/(float)floodset.size();

    //Not High Enough: Fill 'er up
    if(plane + vheight < minbound.second)
      plane += vheight;

    else{
      volume -= (minbound.second - plane)/volumeFactor*(float)floodset.size();
      plane = minbound.second;
    }

    for(auto& s: floodset)
      p[s.x*dim.y+s.y] = plane - h[s.x*dim.y+s.y];

    boundary.erase(minbound.first);
    tried[minbound.first] = false;
    ipos = ivec2(minbound.first/dim.y, minbound.first%dim.y);

  }

  if(drainfound){

    //Search for Exposed Neighbor with Non-Zero Waterlevel
    const std::function<void(glm::ivec2)> lowbound = [&](glm::ivec2 i){

      //Out-Of-Bounds
      if(i.x < 0 || i.y < 0 || i.x >= dim.x || i.y >= dim.y)
        return;

      if(p[i.x*dim.y+i.y] == 0)
        return;

      //Below Drain Height
      if(h[i.x*dim.y+drain.y] + p[i.x*dim.y+drain.y] < h[drain.x*dim.y+drain.y] + p[drain.x*dim.y+drain.y])
        return;

      //Higher than Plane (we want lower)
      if(h[i.x*dim.y+i.y] + p[i.x*dim.y+i.y] >= plane)
        return;

      plane = h[i.x*dim.y+i.y] + p[i.x*dim.y+i.y];

    };

    lowbound(drain+glm::ivec2(1,0));    //Fill Neighbors
    lowbound(drain-glm::ivec2(1,0));    //Fill Neighbors
    lowbound(drain+glm::ivec2(0,1));    //Fill Neighbors
    lowbound(drain-glm::ivec2(0,1));    //Fill Neighbors
    lowbound(drain+glm::ivec2(1,1));    //Fill Neighbors
    lowbound(drain-glm::ivec2(1,1));    //Fill Neighbors
    lowbound(drain+glm::ivec2(-1,1));    //Fill Neighbors
    lowbound(drain-glm::ivec2(-1,1));    //Fill Neighbors

    float oldvolume = volume;

    //Water-Level to Plane-Height
    for(auto& s: floodset){
      int j = s.x*dim.y+s.y;
    //  volume += ((plane > h[ind])?(h[ind] + p[ind] - plane):p[ind])/volumeFactor;
      p[j] = (plane > h[j])?(plane-h[j]):0.0;
    }

    for(auto& b: boundary){
      int j = b.first;
    //  volume += ((plane > h[ind])?(h[ind] + p[ind] - plane):p[ind])/volumeFactor;
      p[j] = (plane > h[j])?(plane-h[j]):0.0;
    }

//    sediment *= oldvolume/volume;
    sediment /= (float)floodset.size(); //Distribute Sediment in Pool
    pos = drain;

    return true;

  }

  return false;

}
