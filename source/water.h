//Small Utility Function
double max(double a, double b){
  return (a > b)?a:b;
}

struct Drop{
  //Construct Particle at Position
  Drop(glm::vec2 _pos){ pos = _pos; }
  Drop(glm::vec2 _p, glm::ivec2 dim, double v){
    pos = _p;
    int index = _p.x*dim.y+_p.y;
    volume = v;
  }

  //Properties
  int index;
  glm::vec2 pos;
  glm::vec2 speed = glm::vec2(0.0);
  double volume = 1.0;   //This will vary in time
  double sediment = 0.0; //Sediment concentration

  //Parameters
  const double density = 1.0;  //This gives varying amounts of inertia and stuff...
  const double evapRate = 0.001;
  const double depositionRate = 1.2*0.08;
  const double minVol = 0.01;
  const double friction = 0.25;
  const double volumeFactor = 0.1; //"Water Deposition Rate"

  //Number of Spills Left
  int spill = 5;

  bool descend(glm::vec3 n, double* h, double* path, double* pool, bool* track, double* pd, glm::ivec2 dim, double scale);
  bool flood(double* h, double* pool, glm::ivec2 dim);

  static void cascade(vec2 pos, glm::ivec2 dim, double* h, double* p){

    ivec2 ipos = pos;
    int ind = ipos.x * dim.y + ipos.y;

    if(p[ind] > 0) return; //Don't do this with water

    //Neighbor Positions (8-Way)
    const int nx[8] = {-1,-1,-1, 0, 0, 1, 1, 1};
    const int ny[8] = {-1, 0, 1,-1, 1,-1, 0, 1};

    const float maxdiff = 0.01f;
    const float settling = 0.2f;

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

bool Drop::descend(glm::vec3 n, double* h, double* p, double* b, bool* track, double* pd, glm::ivec2 dim, double scale){

  if(volume < minVol)
    return false;

  //Initial Position
  glm::ivec2 ipos = pos;
  int ind = ipos.x*dim.y+ipos.y;

  //Add to Path
  track[ind] = true;

  //Effective Parameter Set
  /* Higher plant density means less erosion */
  double effD = depositionRate*max(0.0, 1.0-pd[ind]);

  /* Higher Friction, Lower Evaporation in Streams
  makes particles prefer established streams -> "curvy" */

  double effF = friction*(1.0-p[ind]);
  double effR = evapRate*(1.0-0.2*p[ind]);

  speed = mix(vec2(n.x, n.z), speed, effF);
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

  //Particle is Not ACcelerated
  if(length(vec2(n.x, n.z))*effF < 1E-5)
    return false;

  //Particle enters Pool
  if(b[nind] > 0.0)
    return false;

  //Mass-Transfer (in MASS)
  double c_eq = max(0.0, glm::length(speed)*(h[ind]-h[nind]));
  double cdiff = c_eq - sediment;
  sediment += effD*cdiff;
  h[ind] -= effD*cdiff;

  //Evaporate (Mass Conservative)
  sediment /= (1.0-effR);
  volume *= (1.0-effR);

  cascade(pos, dim, h, b);

  return true;

}

#include <unordered_map>
#include <unordered_set>

bool Drop::flood(double* h, double* p, glm::ivec2 dim){

  if(volume < minVol || spill-- <= 0)
    return false;

  //Dimensions
  const int size = dim.x*dim.y;
  glm::ivec2 ipos = pos;
  int index = ipos.x*dim.y + ipos.y;

  double plane = h[index] + p[index];

  //Flood and Boundary Sets
  vector<int> set;
  unordered_map<int, double> boundary;
  bool tried[size] = {false};

  int drain;
  bool drainfound = false;

  const std::function<void(int)> fill = [&](int i){

    //Out of Bounds
    if(i < 0 || i >= size) return;

    //Position has been tried
    if(tried[i]) return;
    tried[i] = true;

    //Wall / Boundary
    if(plane < h[i] + p[i]){
      boundary[i] = h[i] + p[i];
      return;
    }

    //Drainage Point
    if(plane > h[i] + p[i]){

      //No Drain yet
      if(!drainfound)
        drain = i;

      //Lower Drain
      else if(p[i] + h[i] < p[drain] + h[drain])
        drain = i;

      drainfound = true;
      return;

    }

    //Part of the Pool
    set.push_back(i);
    fill(i+dim.y);    //Fill Neighbors
    fill(i-dim.y);
    fill(i+1);
    fill(i-1);
    fill(i+dim.y+1);  //Diagonals (Improves Drainage)
    fill(i-dim.y-1);
    fill(i+dim.y-1);
    fill(i-dim.y+1);

  };

  fill(index);            //Flood from Initial Position

  //Grow the Pool while Filling!

  while(volume > minVol){

    if(drainfound){

      //Search for Exposed Neighbor with Non-Zero Waterlevel
      const std::function<void(int)> lowbound = [&](int i){

        //Out-Of-Bounds
        if(i < 0 || i >= size)
          return;

        //Below Drain Height
        if(h[i] + p[i] < h[drain] + p[drain])
          return;

        //Higher than Plane (we want lower)
        if(h[i] + p[i] > plane)
          return;

        //No Water Level
        if(p[i] == 0)
          return;

        plane = h[i] + p[i];

      };

      lowbound(drain+dim.y);    //Fill Neighbors
      lowbound(drain-dim.y);
      lowbound(drain+1);
      lowbound(drain-1);
      lowbound(drain+dim.y+1);  //Diagonals (Improves Drainage)
      lowbound(drain-dim.y-1);
      lowbound(drain+dim.y-1);
      lowbound(drain-dim.y+1);

      for(auto& s: set){ //Water Level to Plane Height
        p[s] = (plane > h[s])?(plane-h[s]):0.0;
        volume += ((plane > h[s])?(h[s] + p[s] - plane):p[s])/volumeFactor;
      }

      //Set Position to Drain, Continue
      pos = glm::vec2(drain/dim.y, drain%dim.y);
      return true;

    }

    //Find the Lowest Element on the Boundary
    pair<int, double> minbound = (*boundary.begin());
    for(auto& b : boundary)
    if(b.second < minbound.second)
      minbound = b;

    //Compute the Height of our Volume over the Set
    double vheight = volume*volumeFactor/(double)set.size();

    //Not High Enough: Fill 'er up
    if(plane + vheight < minbound.second){
      plane += vheight;
      break;
    }

    //Raise Plane to Height of Minbound! Remove Volume
    volume -= (minbound.second - plane)/volumeFactor*(double)set.size();
    plane = minbound.second;

    //Remove Minbound from Boundary Set, Fill from Position
    boundary.erase(minbound.first);
    tried[minbound.first] = false;
    fill(minbound.first);

  }

  for(auto& s: set)
    p[s] = plane - h[s];

  return false;

}
