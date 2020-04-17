//FLOW PARTICLE

struct Drop{
  //Construct Particle at Position
  Drop(glm::vec2 _pos){ pos = _pos; }
  Drop(glm::vec2 _p, double v){
    pos = _p;
    int index = _p.x*256+_p.y;
    volume = v;
  }

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
  const float friction = 0.05;
  const float volumeFactor = 150.0;

  //Sedimenation Process
  void process(double* h, double* path, double* pool, glm::vec2 dim, double scale);
  void flood(double* h, double* pool);
};

glm::vec3 surfaceNormal(int index, double* h, double scale){
  glm::vec3 n = glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(h[index]-h[index+256]), 1.0, 0.0));  //Positive X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(h[index-256]-h[index]), 1.0, 0.0));  //Negative X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(h[index]-h[index+1])));    //Positive Y
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(h[index-1]-h[index])));  //Negative Y

  //Diagonals! (This removes the last spatial artifacts)
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index+257])/sqrt(2), sqrt(2), scale*(h[index]-h[index+257])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index+255])/sqrt(2), sqrt(2), scale*(h[index]-h[index+255])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index-255])/sqrt(2), sqrt(2), scale*(h[index]-h[index-255])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(h[index]-h[index-257])/sqrt(2), sqrt(2), scale*(h[index]-h[index-257])/sqrt(2)));    //Positive Y
  return n;
}

void Drop::process(double* h, double* p, double* b, glm::vec2 dim, double scale){

  glm::ivec2 ipos;
  bool path[256*256] = {false};

  while(volume > minVol){

    //Initial Position
    ipos = pos;
    int ind = ipos.x*256+ipos.y;

    //Add to Path
    path[ind] = true;

    glm::vec3 n = surfaceNormal(ind, h, scale);

    //Effective Parameters
    float effD = depositionRate;
    float effF = friction;
    float effR = evapRate;

    //Newtonian Mechanics
    glm::vec2 acc = glm::vec2(n.x, n.z)/(volume*density);
    speed += dt*acc;
    pos   += dt*speed;
    speed *= (1.0-dt*effF);

    //New Position
    int nind = (int)(pos.x)*256+(int)(pos.y);

    //Out-Of-Bounds
    if(!glm::all(glm::greaterThanEqual(pos, glm::vec2(0))) ||
       !glm::all(glm::lessThan(pos, dim))){
         volume = 0.0;
         break;
       }

    //Stopped Particle
    if(length(acc) < 0.01 && length(speed) < 0.01)
      break;

    //Particle ends in Pool
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

  //Update Path
  float lrate = 0.01;
  for(int i = 0; i < 256*256; i++)
    if(path[i]) p[i] = (1.0-lrate)*p[i] + lrate;
    else p[i] *= (1.0-lrate*0.01);
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

void Drop::flood(double* h, double* p){
  //Current Height
  index = (int)pos.x*256 + (int)pos.y;
  double plane = h[index] + p[index];
  double initialplane = plane;

  float tol = 0.001;
  int fail = 10;
  std::vector<int> set; //Floodset

  //Iterate
  while(volume > minVol && fail){
    set.clear();
    bool tried[256*256] = {false};

    int drain;
    bool drainfound = false;

    std::function<void(int)> fill = [&](int i){

      int x = i/256;
      int y = i%256;

      //Out of Bounds
      if(x >= 256 || x < 0) return;
      if(y >= 256 || y < 0) return;

      if(tried[i]) return;  //Position has been tried
      if(plane < h[i] + p[i]) return;  //Plane is lower

      //Find Drain
      if(initialplane > h[i] + p[i]){
        tried[i] = true;

        if(drainfound){
          //Lowest Drain
          if( p[drain] + h[drain] < p[i] + h[i] )
            drain = i;
        }
        else
          drain = i;

        drainfound = true;
        return;
      }

      set.push_back(i);
      tried[i] = true;

      //Fill Neighbors
      fill(i+256);
      fill(i-256);
      fill(i+1);
      fill(i-1);
    };

    //Perform Flood
    fill(index);

    //Drainage Point
    if(drainfound){

      //Set the Drop Position and Evaporate
      pos = glm::vec2(drain/256, drain%256);
      volume *= (1.0-dt*evapRate);

      //Set the New Waterlevel
      float drainage = 0.001;
      plane = (1.0-drainage)*initialplane + drainage*(h[drain] + p[drain]);

      for(auto& s: set){
        //Compute the New Height
        p[s] = plane - h[s];
        if(p[s] < 0.0) p[s] = 0.0;
      }

      break;
    }

    //Strong divergence of the field could mean set is empty.

    //Iterate over the Set
    double tVol = 0.0;  //Get the total volume of the guy

    for(auto& s: set)
      tVol += volumeFactor*(plane - (h[s]+p[s]));

    //Succesful Fill
    if(tVol <= volume && initialplane < plane){

      //Set Volume to Plane Height
      for(auto& s: set)
        p[s] = plane - h[s];

      //Adjust Volume
      volume -= tVol;
      tVol = 0.0;
    }
    else fail--;

    //Adjust Planes
    initialplane = (plane > initialplane)?plane:initialplane;
    plane += 0.5*(volume-tVol)/(float)set.size()/volumeFactor;
  }

  if(fail == 0)
    volume = 0.0;

}
