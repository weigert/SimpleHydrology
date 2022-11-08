
int WSIZE = (2 << 6);
int SCALE = 32;
float MAXDIFF = 0.01f;
int FREQUENCY = 1;

#include "include/math.h"
#include "vegetation.h"
#include "water.h"

struct World{

  //Constructor
  void generate();                      //Initialize Heightmap
  void erode(int cycles);               //Erode with N Particles
  bool grow();
  void incres();                        //Increase the Resolution

  int SEED = 0;
  int AGE = 0;
  glm::ivec2 dim = glm::vec2(WSIZE, WSIZE);  //Size of the heightmap array

  float* heightmap = NULL;    //Flat Array
  float min, max = 0.0;       //Perlin Noise Minima, Maxima

  float* waterpath = NULL;    //Water Path Storage (Rivers)
  float* waterpool = NULL;    //Water Pool Storage (Lakes / Ponds)

  //Trees
  std::vector<Plant> trees;
  float* plantdensity = NULL; //Density for Plants

  //Erosion Process
  bool active = false;

  glm::vec3 normal(glm::vec2 p){

    glm::vec3 n = glm::vec3(0);

    //Two large triangles adjacent to the plane (+Y -> +X) (-Y -> -X)

    if(p.x < dim.x-1 && p.y < dim.y-1)
    n += glm::cross(
      glm::vec3( 0.0, SCALE*(heightmap[math::cflatten(p.x, p.y+1, dim)] - heightmap[math::cflatten(p.x, p.y, dim)] + waterpool[math::cflatten(p.x, p.y+1, dim)] - waterpool[math::cflatten(p.x, p.y, dim)]), 1.0),
      glm::vec3( 1.0, SCALE*(heightmap[math::cflatten(p.x+1, p.y, dim)] - heightmap[math::cflatten(p.x, p.y, dim)] + waterpool[math::cflatten(p.x+1, p.y, dim)] - waterpool[math::cflatten(p.x, p.y, dim)]), 0.0)
    );

    if(p.x > 1 && p.y > 1)
    n += glm::cross(
      glm::vec3( 0.0, SCALE*(heightmap[math::cflatten(p.x, p.y-1, dim)] - heightmap[math::cflatten(p.x, p.y, dim)] + waterpool[math::cflatten(p.x, p.y-1, dim)] - waterpool[math::cflatten(p.x, p.y, dim)]),-1.0),
      glm::vec3(-1.0, SCALE*(heightmap[math::cflatten(p.x-1, p.y, dim)] - heightmap[math::cflatten(p.x, p.y, dim)] + waterpool[math::cflatten(p.x-1, p.y, dim)] - waterpool[math::cflatten(p.x, p.y, dim)]), 0.0)
    );

    //Two Alternative Planes (+X -> -Y) (-X -> +Y)

    if(p.x < dim.x-1 && p.y > 1)
    n += glm::cross(
      glm::vec3( 1.0, SCALE*(heightmap[math::cflatten(p.x+1, p.y, dim)] - heightmap[math::cflatten(p.x, p.y, dim)] + waterpool[math::cflatten(p.x+1, p.y, dim)] - waterpool[math::cflatten(p.x, p.y, dim)]), 0.0),
      glm::vec3( 0.0, SCALE*(heightmap[math::cflatten(p.x, p.y-1, dim)] - heightmap[math::cflatten(p.x, p.y, dim)] + waterpool[math::cflatten(p.x, p.y-1, dim)] - waterpool[math::cflatten(p.x, p.y, dim)]),-1.0)
    );

    if(p.x > 1 && p.y < dim.y-1)
    n += glm::cross(
      glm::vec3(-1.0, SCALE*(heightmap[math::cflatten(p.x-1, p.y, dim)] - heightmap[math::cflatten(p.x, p.y, dim)] + waterpool[math::cflatten(p.x+1, p.y, dim)] - waterpool[math::cflatten(p.x, p.y, dim)]), 0.0),
      glm::vec3( 0.0, SCALE*(heightmap[math::cflatten(p.x, p.y+1, dim)] - heightmap[math::cflatten(p.x, p.y, dim)] + waterpool[math::cflatten(p.x, p.y+1, dim)] - waterpool[math::cflatten(p.x, p.y, dim)]), 1.0)
    );

    return glm::normalize(n);

  }

  World(){
    heightmap = new float[dim.x*dim.y]{0.0f};
    waterpath = new float[dim.x*dim.y]{0.0f};
    waterpool = new float[dim.x*dim.y]{0.0f};
    plantdensity = new float[dim.x*dim.y]{0.0f};
  }

  ~World(){
    if( heightmap != NULL) delete[] heightmap;
    if( waterpath != NULL) delete[] waterpath;
    if( waterpool != NULL) delete[] waterpool;
    if( plantdensity != NULL) delete[] plantdensity;
  }

};

/*
===================================================
          WORLD GENERATING FUNCTIONS
===================================================
*/

void World::generate(){
  std::cout<<"Generating New World"<<std::endl;
  if(SEED == 0) SEED = time(NULL);

  std::cout<<"Seed: "<<SEED<<std::endl;
  //Seed the Random Generator
  srand(SEED);

  std::cout<<"... generating height ..."<<std::endl;

  //Initialize Heightmap
  noise::module::Perlin perlin;

  //Mountainy:
  perlin.SetOctaveCount(8);
  perlin.SetFrequency(FREQUENCY);
  perlin.SetPersistence(0.6);

  min = 0.0;
  max = 0.0;

  for(int x = 0; x < dim.x; x++)
  for(int y = 0; y < dim.y; y++){
    int i = math::cflatten(x, y, dim);
    heightmap[i] = perlin.GetValue((float)x*(1.0/dim.x), (float)y*(1.0/dim.y), SEED);
    if(heightmap[i] > max) max = heightmap[i];
    if(heightmap[i] < min) min = heightmap[i];
  }

  //Normalize
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = (heightmap[i] - min)/(max - min);//+1.0f*((float)((i/dim.y)*(i/dim.y))/dim.x/dim.x)*((float)((i%dim.y)*(i%dim.y))/dim.y/dim.y);
  }

}

void World::incres(){

  // Create Difference Map

  //Initialize Heightmap
  noise::module::Perlin perlin;

  perlin.SetOctaveCount(8);
  perlin.SetFrequency(FREQUENCY);
  perlin.SetPersistence(0.6);

  for(int x = 0; x < dim.x; x++)
  for(int y = 0; y < dim.y; y++){

    int i = math::cflatten(x, y, dim);

    float h = perlin.GetValue((float)x*(1.0/dim.x), (float)y*(1.0/dim.y), SEED);
    h = (h - min)/(max-min);

    heightmap[i] = h - heightmap[i];

  }

  // Scale the New Heightmap

  SCALE *= 2;
  WSIZE *= 2;
  MAXDIFF *= 0.5f;

  dim.x = WSIZE;
  dim.y = WSIZE;

  float* hnew = new float[dim.x*dim.y]{0.0f};

  min = 0.0f;
  max = 0.0f;

  for(int x = 0; x < dim.x; x++)
  for(int y = 0; y < dim.y; y++){

    int i = math::cflatten(x, y, dim);

    hnew[i] = perlin.GetValue((float)x*(1.0/dim.x), (float)y*(1.0/dim.y), SEED);

    if(hnew[i] > max) max = hnew[i];
    if(hnew[i] < min) min = hnew[i];

  }

  for(int x = 0; x < dim.x; x++)
  for(int y = 0; y < dim.y; y++){

    int i = math::cflatten(x, y, dim);
    int oi = math::cflatten(x/2, y/2, dim/2);

    hnew[i] = (hnew[i] - min)/(max - min);
    hnew[i] -= heightmap[oi];

  }

  float* wnew = new float[dim.x*dim.y]{0.0f};

  for(int x = 0; x < dim.x; x++)
  for(int y = 0; y < dim.y; y++){

    int i = math::cflatten(x, y, dim);
    int oi = math::cflatten(x/2, y/2, dim/2);

    wnew[i] = waterpath[oi];

  }

  delete[] heightmap;
  delete[] waterpath;
  delete[] waterpool;
  delete[] plantdensity;

  heightmap = hnew;
  waterpath = wnew;
  waterpool = new float[dim.x*dim.y]{0.0f};
  plantdensity = new float[dim.x*dim.y]{0.0f};
  trees.clear();

}

/*
===================================================
          HYDRAULIC EROSION FUNCTIONS
===================================================
*/
void World::erode(int cycles){

  AGE++;

  //Track the Movement of all Particles
  float track[dim.x*dim.y] = {0.0f};

  //Do a series of iterations!
  for(int i = 0; i < cycles; i++){

    //Spawn New Particle
    glm::vec2 newpos = glm::vec2(rand()%(int)dim.x, rand()%(int)dim.y);
    Drop drop(newpos);

    while(true){

      while(drop.descend(normal(drop.pos), heightmap, waterpath, waterpool, track, plantdensity, dim, SCALE));
    //  if(!drop.flood(heightmap, waterpool, dim))
        break;

    }

  }

  //Update Path
  float lrate = 0.01;
  for(int i = 0; i < dim.x*dim.y; i++)
    waterpath[i] = (1.0-lrate)*waterpath[i] + lrate*50.0f*track[i]/(1.0f + 50.0f*track[i]);

}

bool World::grow(){

  //Random Position
  {
    int i = rand()%(dim.x*dim.y);
    glm::vec3 n = normal(math::cunflatten(i, dim));

    if( waterpool[i] == 0.0 &&
        waterpath[i] < 0.2 &&
        n.y > 0.8 ){

        Plant ntree(i, dim);
        ntree.root(plantdensity, dim, 1.0);
        trees.push_back(ntree);
    }
  }

  //Loop over all Trees
  for(int i = 0; i < trees.size(); i++){

    //Grow the Tree
    trees[i].grow();

    //Spawn a new Tree!
    if(rand()%50 == 0){
      //Find New Position
      glm::vec2 npos = trees[i].pos + glm::ivec2(rand()%9-4, rand()%9-4);

      //Check for Out-Of-Bounds
      if( npos.x >= 0 && npos.x < dim.x &&
          npos.y >= 0 && npos.y < dim.y ){

        Plant ntree(npos, dim);
        glm::vec3 n = normal(math::cunflatten(ntree.index, dim));

        if( waterpool[ntree.index] == 0.0 &&
            waterpath[ntree.index] < 0.2 &&
            n.y > 0.8 &&
            (float)(rand()%1000)/1000.0 > plantdensity[ntree.index]){
              ntree.root(plantdensity, dim, 1.0);
              trees.push_back(ntree);
            }
      }
    }

    //If the tree is in a pool or in a stream, kill it
    if(waterpool[trees[i].index] > 0.0 ||
       waterpath[trees[i].index] > 0.2 ||
       rand()%1000 == 0 ){ //Random Death Chance
         trees[i].root(plantdensity, dim, -1.0);
         trees.erase(trees.begin()+i);
         i--;
       }
  }

  return true;

};
