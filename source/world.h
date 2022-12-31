#include "vegetation.h"

class World{
public:
  //Constructor
  void generate();                      //Initialize Heightmap
  void erode(int cycles);               //Erode with N Particles
  bool grow();

  int SEED = 0;
  glm::ivec2 dim = glm::vec2(WSIZE, WSIZE);  //Size of the heightmap array

  static float heightmap[WSIZE*WSIZE];    //Flat Array

  static float waterpath[WSIZE*WSIZE];    //Water Path Storage (Rivers)
  static float waterpool[WSIZE*WSIZE];    //Water Pool Storage (Lakes / Ponds)

  static float momentumx[WSIZE*WSIZE];    //Momentum X Storage (Rivers)
  static float momentumy[WSIZE*WSIZE];    //Momentum Y Storage (Rivers)

  //Trees
  std::vector<Plant> trees;
  static float plantdensity[WSIZE*WSIZE]; //Density for Plants

  //Erosion Process
  bool active = false;

  glm::vec3 normal(int index){

    //Two large triangels adjacent to the plane (+Y -> +X) (-Y -> -X)
    glm::vec3 n = glm::cross(glm::vec3(0.0, SCALE*(heightmap[index+1]-heightmap[index] + waterpool[index+1] - waterpool[index]), 1.0), glm::vec3(1.0, SCALE*(heightmap[index+dim.y]+waterpool[index+dim.y]-heightmap[index]-waterpool[index]), 0.0));
    n += glm::cross(glm::vec3(0.0, SCALE*(heightmap[index-1]-heightmap[index] + waterpool[index-1]-waterpool[index]), -1.0), glm::vec3(-1.0, SCALE*(heightmap[index-dim.y]-heightmap[index]+waterpool[index-dim.y]-waterpool[index]), 0.0));

    //Two Alternative Planes (+X -> -Y) (-X -> +Y)
    n += glm::cross(glm::vec3(1.0, SCALE*(heightmap[index+dim.y]-heightmap[index]+waterpool[index+dim.y]-waterpool[index]), 0.0), glm::vec3(0.0, SCALE*(heightmap[index-1]-heightmap[index]+waterpool[index-1]-waterpool[index]), -1.0));
    n += glm::cross(glm::vec3(-1.0, SCALE*(heightmap[index-dim.y]-heightmap[index]+waterpool[index-dim.y]-waterpool[index]), 0.0), glm::vec3(0.0, SCALE*(heightmap[index+1]-heightmap[index]+waterpool[index+1]-waterpool[index]), 1.0));

    return glm::normalize(n);

  }


};

float World::heightmap[WSIZE*WSIZE] = {0.0};    //Flat Array

float World::waterpath[WSIZE*WSIZE] = {0.0};    //Water Path Storage (Rivers)
float World::waterpool[WSIZE*WSIZE] = {0.0};    //Water Pool Storage (Lakes / Ponds)

float World::momentumx[WSIZE*WSIZE] = {0.0};    //Momentum X Storage (Rivers)
float World::momentumy[WSIZE*WSIZE] = {0.0};    //Momentum Y Storage (Rivers)

float World::plantdensity[WSIZE*WSIZE] = {0.0}; //Density for Plants

#include "water.h"

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

  float min, max = 0.0;
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = perlin.GetValue((i/dim.y)*(1.0/dim.x), (i%dim.y)*(1.0/dim.y), SEED);
    if(heightmap[i] > max) max = heightmap[i];
    if(heightmap[i] < min) min = heightmap[i];
  }
  //Normalize
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = (heightmap[i] - min)/(max - min);//+1.0f*((float)((i/dim.y)*(i/dim.y))/dim.x/dim.x)*((float)((i%dim.y)*(i%dim.y))/dim.y/dim.y);
  }
}

/*
===================================================
          HYDRAULIC EROSION FUNCTIONS
===================================================
*/
void World::erode(int cycles){

  //Track the Movement of all Particles
  float track[dim.x*dim.y] = {0.0f};
  float mx[dim.x*dim.y] = {0.0f};
  float my[dim.x*dim.y] = {0.0f};

  //Do a series of iterations!
  for(int i = 0; i < cycles; i++){

    //Spawn New Particle
    glm::vec2 newpos = glm::vec2(rand()%(int)dim.x, rand()%(int)dim.y);
    Drop drop(newpos);

    while(true){

      while(drop.descend(normal((int)drop.pos.x * dim.y + (int)drop.pos.y), track, mx, my, dim, SCALE));
      if(!drop.flood(heightmap, waterpool, dim))
        break;

    }

  }

  //Update Path
  float lrate = 0.01;
  for(int i = 0; i < dim.x*dim.y; i++){

    waterpath[i] = (1.0-lrate)*waterpath[i] + lrate*track[i];///(1.0f + 50.0f*track[i]);

    momentumx[i] = (1.0-lrate)*momentumx[i] + lrate*mx[i];
    momentumy[i] = (1.0-lrate)*momentumy[i] + lrate*my[i];

  }

}

bool World::grow(){

  //Random Position
  {
    int i = rand()%(dim.x*dim.y);
    glm::vec3 n = normal(i);

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
      glm::vec2 npos = trees[i].pos + glm::vec2(rand()%9-4, rand()%9-4);

      //Check for Out-Of-Bounds
      if( npos.x >= 0 && npos.x < dim.x &&
          npos.y >= 0 && npos.y < dim.y ){

        Plant ntree(npos, dim);
        glm::vec3 n = normal(ntree.index);

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
       waterpath[trees[i].index] > 0.5 ||
       rand()%1000 == 0 ){ //Random Death Chance
         trees[i].root(plantdensity, dim, -1.0);
         trees.erase(trees.begin()+i);
         i--;
       }
  }

  return true;

};
