#include "vegetation.h"
#include "water.h"

class World{
public:
  //Constructor
  void generate();                      //Initialize Heightmap
  void erode(int cycles);               //Erode with N Particles
  bool grow();

  int SEED = 0;
  glm::ivec2 dim = glm::vec2(WSIZE, WSIZE);  //Size of the heightmap array

  double scale = 80.0;                  //"Physical" Height scaling of the map
  double heightmap[WSIZE*WSIZE] = {0.0};    //Flat Array

  double waterpath[WSIZE*WSIZE] = {0.0};    //Water Path Storage (Rivers)
  double waterpool[WSIZE*WSIZE] = {0.0};    //Water Pool Storage (Lakes / Ponds)

  //Trees
  std::vector<Plant> trees;
  double plantdensity[WSIZE*WSIZE] = {0.0}; //Density for Plants

  //Erosion Process
  bool active = false;
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
  perlin.SetFrequency(1.0);
  perlin.SetPersistence(0.5);

  double min, max = 0.0;
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = perlin.GetValue((i/dim.y)*(1.0/dim.x), (i%dim.y)*(1.0/dim.y), SEED);
    if(heightmap[i] > max) max = heightmap[i];
    if(heightmap[i] < min) min = heightmap[i];
  }
  //Normalize
  for(int i = 0; i < dim.x*dim.y; i++){
    heightmap[i] = (heightmap[i] - min)/(max - min);
  }
}

/*
===================================================
          HYDRAULIC EROSION FUNCTIONS
===================================================
*/
void World::erode(int cycles){

  //Track the Movement of all Particles
  bool track[dim.x*dim.y] = {false};

  //Do a series of iterations!
  for(int i = 0; i < cycles; i++){

    //Spawn New Particle
    glm::vec2 newpos = glm::vec2(rand()%(int)dim.x, rand()%(int)dim.y);
    Drop drop(newpos);


    int spill = 5;
    while(drop.volume > drop.minVol && spill != 0){

      drop.descend(heightmap, waterpath, waterpool, track, plantdensity, dim, scale);

      if(drop.volume > drop.minVol)
        drop.flood(heightmap, waterpool, dim);

      spill--;
    }
  }

  //Update Path
  double lrate = 0.01;
  for(int i = 0; i < dim.x*dim.y; i++)
    waterpath[i] = (1.0-lrate)*waterpath[i] + lrate*((track[i])?1.0:0.0);

}

bool World::grow(){

  //Random Position
  {
    int i = rand()%(dim.x*dim.y);
    glm::vec3 n = surfaceNormal(i, heightmap, waterpool, dim, scale);

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
        glm::vec3 n = surfaceNormal(ntree.index, heightmap, waterpool, dim, scale);

        if( waterpool[ntree.index] == 0.0 &&
            waterpath[ntree.index] < 0.2 &&
            n.y > 0.8 &&
            (double)(rand()%1000)/1000.0 > plantdensity[ntree.index]){
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
