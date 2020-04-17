//FLOW PARTICLE

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
  float volume = 1.0;   //This will vary in time
  float sediment = 0.0; //Fraction of Volume that is Sediment!

  //Parameters
  const float dt = 1.2;
  const float density = 1.0;  //This gives varying amounts of inertia and stuff...
  const float evapRate = 0.001;
  const float depositionRate = 0.1;
  const float minVol = 0.01;
  const float friction = 0.1;
  const float volumeFactor = 100.0; //"Water Deposition Rate"

  //Sedimenation Process
  void process(double* h, double* path, double* pool, bool* track, glm::ivec2 dim, double scale);
  void flood(double* h, double* pool, glm::ivec2 dim);
};

glm::vec3 surfaceNormal(int index, double* h, glm::ivec2 dim, double scale){
  glm::vec3 n = glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(h[index]-h[index+dim.y]), 1.0, 0.0));  //Positive X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(h[index-dim.y]-h[index]), 1.0, 0.0));  //Negative X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(h[index]-h[index+1])));    //Positive Y
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(h[index-1]-h[index])));  //Negative Y

  //Diagonals! (This removes the last spatial artifacts)
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index+dim.y+1])/sqrt(2), sqrt(2), scale*(h[index]-h[index+dim.y+1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index+dim.y-1])/sqrt(2), sqrt(2), scale*(h[index]-h[index+dim.y-1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index-dim.y+1])/sqrt(2), sqrt(2), scale*(h[index]-h[index-dim.y+1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index-dim.y-1])/sqrt(2), sqrt(2), scale*(h[index]-h[index-dim.y-1])/sqrt(2)));    //Positive Y
  return n;
}

void Drop::process(double* h, double* p, double* b, bool* track, glm::ivec2 dim, double scale){

  glm::ivec2 ipos;

  while(volume > minVol){

    //Initial Position
    ipos = pos;
    int ind = ipos.x*dim.y+ipos.y;

    //Add to Path
    track[ind] = true;

    glm::vec3 n = surfaceNormal(ind, h, dim, scale);

    //Effective Parameter Set
    float effD = depositionRate;
    /*Lower Friction, Lower Evaporation in Streams
    makes particles prefer established streams -> "curvy"*/
    float effF = friction*(1.0-0.5*p[ind]);
    float effR = evapRate*(1.0-0.2*p[ind]);

    //Newtonian Mechanics
    glm::vec2 acc = glm::vec2(n.x, n.z)/(volume*density);
    speed += dt*acc;
    pos   += dt*speed;
    speed *= (1.0-dt*effF);

    //New Position
    int nind = (int)(pos.x)*dim.y+(int)(pos.y);

    //Out-Of-Bounds
    if(!glm::all(glm::greaterThanEqual(pos, glm::vec2(0))) ||
       !glm::all(glm::lessThan((glm::ivec2)pos, dim))){
         volume = 0.0;
         break;
       }

    //Particle is not accelerated
    if(p[nind] > 0.5 && length(acc) < 0.01)
      break;

    //Particle enters Pool
    if(b[nind] > 0.0)
      break;

    //Mass-Transfer
    float c_eq = volume*glm::length(speed)*(h[ind]-h[nind]);
    if(c_eq < 0.0) c_eq = 0.0;
    float cdiff = c_eq - sediment;
    sediment += dt*effD*cdiff;
    h[ind] -= dt*volume*effD*cdiff;

    //Evaporate
    volume *= (1.0-dt*effR);
  }
};

/*
  A point that is below the testing plane is part of the fill.
  A point that is ALSO below the initial plane is a drainage spot.

  If we find a drainage spot:
    Don't add it to the set, don't test it's neighbors.
    It is now marked as "tested". This way we don't flood beyond this spot.
    Mark it as a drainage spot.
    Continue filling the other areas into the set.

  Once all drains are identified:

  If we still have more particle volume than the test-volume:
    Set the plane to the height of the lowest drain.
    Compute the volume that can be added to the set, subtract it.
    Place the rest as particles at the location of the drain.

  Important:
    The initial Plane is nudged upwards correctly after every iteration
    And it doesn't matter if the droplet position has water or not
    in order to identify a neighboring drain spot.
*/

void Drop::flood(double* h, double* p, glm::ivec2 dim){

  //Current Height
  index = (int)pos.x*dim.y + (int)pos.y;
  double plane = h[index] + p[index];
  double initialplane = plane;

  //Floodset
  std::vector<int> set;
  int fail = 10;

  //Iterate
  while(volume > minVol && fail){

    set.clear();
    bool tried[dim.x*dim.y] = {false};

    int drain;
    bool drainfound = false;

    std::function<void(int)> fill = [&](int i){

      //Out of Bounds
      if(i/dim.y >= dim.x || i/dim.y < 0) return;
      if(i%dim.y >= dim.y || i%dim.y < 0) return;

      //Position has been tried
      if(tried[i]) return;
      tried[i] = true;

      //Wall / Boundary
      if(plane < h[i] + p[i]) return;

      //Drainage Point
      if(initialplane > h[i] + p[i]){

        //No Drain yet
        if(!drainfound)
          drain = i;

        //Lower Drain
        else if( p[drain] + h[drain] < p[i] + h[i] )
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

    //Perform Flood
    fill(index);

    //Drainage Point
    if(drainfound){

      //Set the Drop Position and Evaporate
      pos = glm::vec2(drain/dim.y, drain%dim.y);

      //Set the New Waterlevel (Slowly)
      float drainage = 0.001;
      plane = (1.0-drainage)*initialplane + drainage*(h[drain] + p[drain]);

      //Compute the New Height
      for(auto& s: set)
        p[s] = (plane > h[s])?(plane-h[s]):0.0;

      //Remove Sediment
      sediment = 0.0;
      break;
    }

    //Get Volume under Plane
    double tVol = 0.0;
    for(auto& s: set)
      tVol += volumeFactor*(plane - (h[s]+p[s]));

    //We can partially fill this volume
    if(tVol <= volume && initialplane < plane){

      //Raise water level to plane height
      for(auto& s: set)
        p[s] = plane - h[s];

      //Adjust Drop Volume
      volume -= tVol;
      tVol = 0.0;
    }

    //Plane was too high.
    else fail--;

    //Adjust Planes
    initialplane = (plane > initialplane)?plane:initialplane;
    plane += 0.5*(volume-tVol)/(float)set.size()/volumeFactor;
  }

  //Couldn't place the volume (for some reason)- so ignore this drop.
  if(fail == 0)
    volume = 0.0;
}
