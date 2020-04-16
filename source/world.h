class World{
public:
  //Constructor
  void generate();                        //Initialize Heightmap
  void erode(int cycles);                 //Perform n erosion cycles
  glm::vec3 surfaceNormal(int i, int j);

  int SEED = 10;
  std::chrono::milliseconds tickLength = std::chrono::milliseconds(1000);
  glm::vec2 dim = glm::vec2(256, 256);  //Size of the heightmap array
  bool updated = false;                 //Flag for remeshing

  double scale = 60.0;                  //"Physical" Height scaling of the map
  double heightmap[256][256] = {0.0};

  //Hydraulic Maps
  double frequencyweight = 4000.0; //Weight of frequency...
  float waterpath[256*256] = {0};
  float waterpool[256*256] = {0};
  double sealevel = 16.0;

  //Erosion Steps
  bool active = false;
  int remaining = 200000;
  int erosionstep = 1000;

  //Particle Properties
  float dt = 1.4;
  float density = 1.0;  //This gives varying amounts of inertia and stuff...
  float evapRate = 0.01;
  float depositionRate = 0.01;
  float minVol = 0.01;
  float friction = 0.05;
};

struct Particle{
  //Construct Particle at Position
  Particle(glm::vec2 _pos){ pos = _pos; }

  glm::vec2 pos;
  glm::vec2 speed = glm::vec2(0.0);

  float volume = 1.0;   //This will vary in time
  float sediment = 0.0; //Fraction of Volume that is Sediment!
};

/*
===================================================
          WORLD GENERATING FUNCTIONS
===================================================
*/

void World::generate(){
  std::cout<<"Generating New World"<<std::endl;
  SEED = time(NULL);
  std::cout<<"Seed: "<<SEED<<std::endl;
  //Seed the Random Generator
  srand(SEED);

  std::cout<<"... generating height ..."<<std::endl;

  //Initialize Heightmap
  noise::module::Perlin perlin;

  //Less Mountainy
  perlin.SetOctaveCount(8);
  perlin.SetFrequency(1.0);
  perlin.SetPersistence(0.4);

  /*
  //Mountainy:
  perlin.SetOctaveCount(8);
  perlin.SetFrequency(1.0);
  perlin.SetPersistence(0.4);

  */

  float min, max = 0.0;
  for(int i = 0; i < dim.x; i++){
    for(int j = 0; j < dim.y; j++){
      heightmap[i][j] = perlin.GetValue(i*(1.0/dim.x), j*(1.0/dim.y), SEED);
      if(heightmap[i][j] > max) max = heightmap[i][j];
      if(heightmap[i][j] < min) min = heightmap[i][j];
    }
  }

  //Normalize
  for(int i = 0; i < dim.x; i++){
    for(int j = 0; j < dim.y; j++){
      //Normalize to (0, 1) scale.
      heightmap[i][j] = (heightmap[i][j] - min)/(max - min);
    }
  }

  //Construct all Triangles...
  updated = true;
}

/*
===================================================
          HYDRAULIC EROSION FUNCTIONS
===================================================
*/

glm::vec3 World::surfaceNormal(int i, int j){
  /*
    Note: Surface normal is computed in this way, because the square-grid surface is meshed using triangles.
    To avoid spatial artifacts, you need to weight properly with all neighbors.
  */

  glm::vec3 n = glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i+1][j]), 1.0, 0.0));  //Positive X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(scale*(heightmap[i-1][j]-heightmap[i][j]), 1.0, 0.0));  //Negative X
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(heightmap[i][j]-heightmap[i][j+1])));    //Positive Y
  n += glm::vec3(0.15) * glm::normalize(glm::vec3(0.0, 1.0, scale*(heightmap[i][j-1]-heightmap[i][j])));  //Negative Y

  //Diagonals! (This removes the last spatial artifacts)
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i+1][j+1])/sqrt(2), sqrt(2), scale*(heightmap[i][j]-heightmap[i+1][j+1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i+1][j-1])/sqrt(2), sqrt(2), scale*(heightmap[i][j]-heightmap[i+1][j-1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i-1][j+1])/sqrt(2), sqrt(2), scale*(heightmap[i][j]-heightmap[i-1][j+1])/sqrt(2)));    //Positive Y
  n += glm::vec3(0.1) * glm::normalize(glm::vec3(scale*(heightmap[i][j]-heightmap[i-1][j-1])/sqrt(2), sqrt(2), scale*(heightmap[i][j]-heightmap[i-1][j-1])/sqrt(2)));    //Positive Y

  return n;
}

void World::erode(int cycles){

  /*
    Note: Everything is properly scaled by a time step-size "dt"
  */

  //Particle Paths
  int _path[256*256] = {0};
  float _pool[256*256] = {0};

  //Do a series of iterations!
  for(int i = 0; i < cycles; i++){

    //Spawn New Particle
    glm::vec2 newpos = glm::vec2(rand()%(int)dim.x, rand()%(int)dim.y);
    Particle drop(newpos);
    glm::ivec2 ipos;

    //As long as the droplet exists...
    while(drop.volume > minVol){

      //Get Position and Surface Normal
      ipos = drop.pos; //Floored Droplet Initial Position
      glm::vec3 n = surfaceNormal(ipos.x, ipos.y);  //Surface Normal at Position
      //Compute Effective Parameter Set
      float effD = depositionRate*(1.0+waterpath[(int)(ipos.x*dim.y+ipos.y)]);
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
        _pool[(int)(ipos.x*dim.y+ipos.y)] += dt*drop.volume;
        break;
      }

      //Check if the Particle enters a pool, add it to the pool!
      if(waterpool[(int)(ipos.x*dim.y+ipos.y)] != 0.0){
        _pool[(int)(ipos.x*dim.y+ipos.y)] += dt*drop.volume;
        break;
      }

      //Check if Particle is still in-bounds
      if(!glm::all(glm::greaterThanEqual(drop.pos, glm::vec2(0))) ||
         !glm::all(glm::lessThan(drop.pos, dim))) break;

     //Add the Particles Position to the Heatmap!
     _path[(int)(ipos.x*dim.y+ipos.y)] = 1;

      //Compute sediment capacity difference
      float c_eq = drop.volume*glm::length(drop.speed)*(heightmap[ipos.x][ipos.y]-heightmap[(int)drop.pos.x][(int)drop.pos.y]);
      if(c_eq < 0.0) c_eq = 0.0;
      float cdiff = c_eq - drop.sediment;

      //Act on the Heightmap and Droplet!
      drop.sediment += dt*effD*cdiff;
      heightmap[ipos.x][ipos.y] -= dt*drop.volume*effD*cdiff;

      //Evaporate the Droplet (Note: Proportional to Volume! Better: Use shape factor to make proportional to the area instead.)
      drop.volume *= (1.0-dt*effR);
    }
  }

  float lrate = 0.01;
  for(int i = 0; i < 256*256; i++){
    //What is the maximum number of times a square can be visited? A bunch I guess.
    waterpath[i] = (1.0-lrate)*waterpath[i]+lrate*_path[i];
  }

  /*
  FLOODING ALGORITHM
  */

  for(int i = 0; i < 256*256; i++){
    if(_pool[i] == 0.0) continue; //No volume to deposit
    float tol = 0.001;

    //The plane starts at the height of position + droplet volume
    float plane0 = heightmap[i/256][i%256] + waterpool[i];  //Initial Plane!
    float plane = plane0; //Testing Plane!

    //Set of connected points
    std::vector<int> _set;      //Part of the Set
    std::vector<int> _drains;   //Drainage Spots

    bool complete = false;
    int maxtries = 100;
    while(maxtries > 0){

      _set.clear();
      _drains.clear();

      bool tried[256*256] = {false};

      //Fill Operation: Identify all positions that are connected and fulfill a criterion
      std::function<void(int,int)> fill = [&](int x, int y){
        //Out of Bounds
        if(x >= 256 || x < 0) return;
        if(y >= 256 || y < 0) return;

        //Other Conditions
        int ind = x*dim.y + y;  //1D Index
        if(tried[ind]) return;  //Position has been tried
        if(plane < heightmap[x][y] + waterpool[ind]) return;  //Plane is lower

        tried[ind] = true;

        //Check for drains
        /*
        if(plane0 > heightmap[x][y] + waterpool[ind]){
          _drains.push_back(ind);
          return;
        }
        */

        //Add to Set!
        _set.push_back(ind);

        //Fill Neighbors
        fill(x+1, y);
        fill(x-1, y);
        fill(x, y+1);
        fill(x, y-1);
      };

      //We need to

      //Fill the Set
      fill(i/256, i%256);
      if(_set.empty()) break; //If we e.g. lower the plane, because we are on an edge.
      float pVol = 0.0;  //Particle Volume in Set
      float tVol = 0.0;  //Total Volume in Set

      std::cout<<_set.size()<<std::endl;

      //Compute over Plane
      for(auto& ind: _set){
        pVol += 0.01*_pool[ind]; //Particle Volume under Plane
        tVol += plane - (heightmap[ind/256][ind%256] + waterpool[ind]);
      }

      //

/*
      std::cout<<"SET: "<<_set.size()<<std::endl;
      std::cout<<"F: "<<plane<<std::endl;
      std::cout<<"H: "<<heightmap[i/256][i%256]+waterpool[i]<<std::endl;
      std::cout<<"P: "<<pVol<<std::endl;
      std::cout<<"T: "<<tVol<<std::endl;
*/

      if(abs(pVol - tVol) < tol){
        for(auto& ind: _set){ //Raise water level
          _pool[ind] = 0.0; //Particle Volume under Plane
          waterpool[ind] = (plane - heightmap[ind/256][ind%256]);
        }
        break;  //Finished
      }

      //Move the Plane Upwards
      plane += 0.1*(pVol-tVol)/(float)_set.size();

      if(--maxtries == 0)
        std::cout<<"Failed number of tries"<<std::endl;
    }
  }
}

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
*/

/*
===================================================
                MESHING FUNCTION
===================================================
*/

int WIDTH = 1000;
int HEIGHT = 1000;

bool paused = true;

float zoom = 0.2;
float zoomInc = 0.01;

//Rotation and View
float rotation = 0.0f;
glm::vec3 cameraPos = glm::vec3(50, 50, 50);
glm::vec3 lookPos = glm::vec3(0, 0, 0);
glm::mat4 camera = glm::lookAt(cameraPos, lookPos, glm::vec3(0,1,0));
glm::mat4 projection = glm::ortho(-(float)WIDTH*zoom, (float)WIDTH*zoom, -(float)HEIGHT*zoom, (float)HEIGHT*zoom, -800.0f, 500.0f);

//Shader Stuff
float steepness = 0.8;
//glm::vec3 flatColor = glm::vec3(0.27, 0.64, 0.27);
//glm::vec3 steepColor = glm::vec3(0.7);
glm::vec3 flatColor = glm::vec3(1.0, 0.709, 0.329);
glm::vec3 steepColor = glm::vec3(0.847, 0.714, 0.592);
glm::vec3 waterColor = glm::vec3(0.086, 0.435, 0.494);

//Lighting and Shading
glm::vec3 skyCol = glm::vec4(0.3, 0.3f, 1.0f, 1.0f);
glm::vec3 lightPos = glm::vec3(-100.0f, 100.0f, -150.0f);
glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 0.9f);
float lightStrength = 1.4;
glm::mat4 depthModelMatrix = glm::mat4(1.0);
glm::mat4 depthProjection = glm::ortho<float>(-300, 300, -300, 300, 0, 800);
glm::mat4 depthCamera = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0,1,0));

glm::mat4 biasMatrix = glm::mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
);

World world;

std::function<void(Model* m)> constructor = [&](Model* m){
  //Clear the Containers
  m->indices.clear();
  m->positions.clear();
  m->normals.clear();
  m->colors.clear();

  //Loop over all positions and add the triangles!
  for(int i = 0; i < world.dim.x-1; i++){
    for(int j = 0; j < world.dim.y-1; j++){

      //Add to Position Vector
      glm::vec3 a = glm::vec3(i, world.scale*world.heightmap[i][j], j);
      glm::vec3 b = glm::vec3(i+1, world.scale*world.heightmap[i+1][j], j);
      glm::vec3 c = glm::vec3(i, world.scale*world.heightmap[i][j+1], j+1);
      glm::vec3 d = glm::vec3(i+1, world.scale*world.heightmap[i+1][j+1], j+1);

      //Add the Pool Height
      bool water = (world.waterpool[(int)(i*world.dim.y+j)] > 0.0);
      a += glm::vec3(0.0, world.scale*world.waterpool[(int)(i*world.dim.y+j)], 0.0);
      b += glm::vec3(0.0, world.scale*world.waterpool[(int)((i+1)*world.dim.y+j)], 0.0);
      c += glm::vec3(0.0, world.scale*world.waterpool[(int)(i*world.dim.y+j+1)], 0.0);
      d += glm::vec3(0.0, world.scale*world.waterpool[(int)((i+1)*world.dim.y+j+1)], 0.0);

      //UPPER TRIANGLE

      //Get the Color of the Ground (Water vs. Flat)
      glm::vec3 color;
      float p = world.waterpath[(int)(i*world.dim.y+j)];
      if(water) color = waterColor;
      else color = glm::mix(flatColor, waterColor, ease::sharpen(p,2));

      //Add Indices
      m->indices.push_back(m->positions.size()/3+0);
      m->indices.push_back(m->positions.size()/3+1);
      m->indices.push_back(m->positions.size()/3+2);

      m->positions.push_back(a.x);
      m->positions.push_back(a.y);
      m->positions.push_back(a.z);
      m->positions.push_back(b.x);
      m->positions.push_back(b.y);
      m->positions.push_back(b.z);
      m->positions.push_back(c.x);
      m->positions.push_back(c.y);
      m->positions.push_back(c.z);

      glm::vec3 n1 = glm::normalize(glm::cross(a-b, c-b));

      for(int i = 0; i < 3; i++){
        m->normals.push_back(n1.x);
        m->normals.push_back(n1.y);
        m->normals.push_back(n1.z);

        //Add the Color!
        if(n1.y < steepness){
          m->colors.push_back(steepColor.x);
          m->colors.push_back(steepColor.y);
          m->colors.push_back(steepColor.z);
          m->colors.push_back(1.0);
        }
        else{
          m->colors.push_back(color.x);
          m->colors.push_back(color.y);
          m->colors.push_back(color.z);
          m->colors.push_back(1.0);
        }

      }

      //Lower Triangle

      m->indices.push_back(m->positions.size()/3+0);
      m->indices.push_back(m->positions.size()/3+1);
      m->indices.push_back(m->positions.size()/3+2);

      m->positions.push_back(d.x);
      m->positions.push_back(d.y);
      m->positions.push_back(d.z);
      m->positions.push_back(c.x);
      m->positions.push_back(c.y);
      m->positions.push_back(c.z);
      m->positions.push_back(b.x);
      m->positions.push_back(b.y);
      m->positions.push_back(b.z);

      glm::vec3 n2 = glm::normalize(glm::cross(d-c, b-c));

      for(int i = 0; i < 3; i++){
        m->normals.push_back(n2.x);
        m->normals.push_back(n2.y);
        m->normals.push_back(n2.z);

        if(n2.y < steepness){
          m->colors.push_back(steepColor.x);
          m->colors.push_back(steepColor.y);
          m->colors.push_back(steepColor.z);
          m->colors.push_back(1.0);
        }
        else{
          m->colors.push_back(color.x);
          m->colors.push_back(color.y);
          m->colors.push_back(color.z);
          m->colors.push_back(1.0);
        }

      }
    }
  }
};

std::function<void()> eventHandler = [&](){

  if(!Tiny::event.scroll.empty()){

    if(Tiny::event.scroll.back().wheel.y > 0.99 && zoom <= 0.3){
      zoom += zoomInc;
      projection = glm::ortho(-(float)WIDTH*zoom, (float)WIDTH*zoom, -(float)HEIGHT*zoom, (float)HEIGHT*zoom, -800.0f, 500.0f);
    }
    else if(Tiny::event.scroll.back().wheel.y < -0.99 && zoom > 0.005){
      zoom -= zoomInc;
      projection = glm::ortho(-(float)WIDTH*zoom, (float)WIDTH*zoom, -(float)HEIGHT*zoom, (float)HEIGHT*zoom, -800.0f, 500.0f);
    }
    else if(Tiny::event.scroll.back().wheel.x < -0.8){
      rotation += 1.5f;
      camera = glm::rotate(camera, glm::radians(1.5f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if(Tiny::event.scroll.back().wheel.x > 0.8){
      rotation -= 1.5f;
      camera = glm::rotate(camera, glm::radians(-1.5f), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    //Adjust Stuff
    if(rotation < 0.0) rotation = 360.0 + rotation;
    else if(rotation > 360.0) rotation = rotation - 360.0;
    camera = glm::rotate(glm::lookAt(cameraPos, lookPos, glm::vec3(0,1,0)), glm::radians(rotation), glm::vec3(0,1,0));
    Tiny::event.scroll.pop_back();
  }

  //
  if(!Tiny::event.keys.empty()){
    if(Tiny::event.keys.back().key.keysym.sym == SDLK_p){
      paused = !paused;
    }

    //Remove the guy
    Tiny::event.keys.pop_back();
  }
};

std::function<glm::vec4(float)> pathColor = [](float t){
  return glm::mix(glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(0.8, 0.0, 0.0, 1.0), ease::sharpen(t, 2));
};

std::function<glm::vec4(float)> poolColor = [](float t){
  return glm::mix(glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), (t > 0.0)?1.0:0.0);
};
