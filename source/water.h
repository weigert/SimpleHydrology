//FLOW PARTICLE

struct Particle{
  //Construct Particle at Position
  Particle(glm::vec2 _pos){ pos = _pos; }

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

  //Sedimenation Process
  void process(double& h, double scale);
};


glm::vec3 normal(int i, int j, double* h, glm::vec2 dim, double scale){
  /*
    Note: Surface normal is computed in this way, because the square-grid surface is meshed using triangles.
    To avoid spatial artifacts, you need to weight properly with all neighbors.
  */
  glm::vec3 n = glm::vec3(0.0);
/*
  glm::vec3 n = glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i+1][j]), 1.0, 0.0));  //Positive X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(heightmap[i-1][j]-heightmap[i][j]), 1.0, 0.0));  //Negative X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(heightmap[i][j]-heightmap[i][j+1])));    //Positive Y
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(heightmap[i][j-1]-heightmap[i][j])));  //Negative Y

  //Diagonals! (This removes the last spatial artifacts)
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i+1][j+1])/sqrt(2), sqrt(2), scale*(heightmap[i][j]-heightmap[i+1][j+1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i+1][j-1])/sqrt(2), sqrt(2), scale*(heightmap[i][j]-heightmap[i+1][j-1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i-1][j+1])/sqrt(2), sqrt(2), scale*(heightmap[i][j]-heightmap[i-1][j+1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i-1][j-1])/sqrt(2), sqrt(2), scale*(heightmap[i][j]-heightmap[i-1][j-1])/sqrt(2)));    //Positive Y
*/
  return n;
}

void Particle::process(double& h, double scale){
  /*

  glm::ivec2 ipos;

  //Descend with Sedimentation
  while(volume > minVol){

    ipos = pos;

    glm::vec3 n = surfaceNormal(ipos.x, ipos.y, h, scale);  //Surface Normal at Position
    //Compute Effective Parameter Set
    float effD = depositionRate;//*(1.0+waterpath[(int)(ipos.x*dim.y+ipos.y)]);
    float effF = friction;//*(1.0-waterpath[(int)(ipos.x*dim.y+ipos.y)]);
    float effR = evapRate;//*(1.0-waterpath[(int)(ipos.x*dim.y+ipos.y)]);

    //Accelerate particle using newtonian mechanics using the surface normal.
    glm::vec2 acc = glm::vec2(n.x, n.z)/(drop.volume*density);
    drop.speed += dt*acc;//F = ma, so a = F/m
    drop.pos   += dt*drop.speed;
    drop.speed *= (1.0-dt*effF);       //Friction Factor

    //Check if the Particle is Stopped
    if(length(acc) < 0.01 && length(drop.speed) < 0.01){
      //Add the Drop Volume to the Pool
      _pool.push_back(Drop(ipos, dt*drop.volume));
      break;
    }

    //Check if the Particle enters a pool, add it to the pool!


    //Check if Particle is still in-bounds
    if(!glm::all(glm::greaterThanEqual(drop.pos, glm::vec2(0))) ||
       !glm::all(glm::lessThan(drop.pos, dim))) break;

   //Add the Particles Position to the Heatmap!
   //_path[(int)(ipos.x*dim.y+ipos.y)] = 1;

    //Compute sediment capacity difference
    float c_eq = drop.volume*glm::length(drop.speed)*(h[ipos.x][ipos.y]-heightmap[drop.pos.x][drop.pos.y]);
    if(c_eq < 0.0) c_eq = 0.0;
    float cdiff = c_eq - drop.sediment;

    //Act on the Heightmap and Droplet!
    drop.sediment += dt*effD*cdiff;
    heightmap[ipos.x][ipos.y] -= dt*drop.volume*effD*cdiff;

    //Evaporate the Droplet (Note: Proportional to Volume! Better: Use shape factor to make proportional to the area instead.)
    drop.volume *= (1.0-dt*effR);
  }
  */
};

//POOL PARTICLE

struct Drop{
  Drop(glm::vec2 _p, double v){
    pos = _p;
    int index = _p.x*256+_p.y;
    volume = v;
  }
  glm::ivec2 pos;
  int index;
  double volume;
};

//These will be unified later.
