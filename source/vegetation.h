#ifndef SIMPLEHYDROLOGY_VEGETATION
#define SIMPLEHYDROLOGY_VEGETATION

struct Plant{
  Plant(int i, glm::ivec2 d){
    index = i;
    pos = glm::vec2(i/d.y, i%d.y);
  };

  Plant(glm::vec2 p, glm::ivec2 d){
    pos = p;
    index = (int)p.x*d.y+(int)p.y;
  };

  glm::vec2 pos;
  int index;
  float size = 0.5;
  const float maxsize = 1.0;
  const float rate = 0.05;

  void grow();
  void root(float* density, glm::ivec2 dim, float factor);

  Plant& operator=(const Plant& o){
    if(this != &o){  //Self Check
      pos = o.pos;
      index = o.index;
      size = o.size;
    }
    return *this;
  };
};


void Plant::grow(){
  size += rate*(maxsize-size);
};

void Plant::root(float* density, glm::ivec2 dim, float f){

  //Can always do this one
  density[index]       += f*1.0;

  if(pos.x > 0){
    //
    density[index - WSIZE] += f*0.6;      //(-1, 0)

    if(pos.y > 0)
      density[index - WSIZE-1] += f*0.4;    //(-1, -1)

    if(pos.y < WSIZE-1)
      density[index - WSIZE+1] += f*0.4;    //(-1, 1)
  }

  if(pos.x < WSIZE-1){
    //
    density[index + WSIZE] += f*0.6;    //(1, 0)

    if(pos.y > 0)
      density[index + WSIZE-1] += f*0.4;    //(1, -1)

    if(pos.y < WSIZE-1)
      density[index + WSIZE+1] += f*0.4;    //(1, 1)
  }

  if(pos.y > 0)
    density[index - 1]   += f*0.6;    //(0, -1)

  if(pos.y < WSIZE-1)
    density[index + 1]   += f*0.6;    //(0, 1)
}

struct Vegetation {

  static std::vector<Plant> plants;
  static float density[WSIZE*WSIZE];     //Density for Plants
  static bool grow();

};

std::vector<Plant> Vegetation::plants;
float Vegetation::density[WSIZE*WSIZE]{0.0f};

bool Vegetation::grow(){

  //Random Position
  {

    int i = rand()%(World::dim.x*World::dim.y);
    glm::vec3 n = World::normal(i);

    if( World::discharge[i] < 0.2 &&
        n.y > 0.8 ){

        Plant nplant(i, World::dim);
        nplant.root(density, World::dim, 1.0);
        plants.push_back(nplant);

    }

  }

  //Loop over all Trees
  for(int i = 0; i < plants.size(); i++){

    //Grow the Tree
    plants[i].grow();

    //Spawn a new Tree!
    if(rand()%50 == 0){
      //Find New Position
      glm::vec2 npos = plants[i].pos + glm::vec2(rand()%9-4, rand()%9-4);

      //Check for Out-Of-Bounds
      if( npos.x >= 0 && npos.x < World::dim.x &&
          npos.y >= 0 && npos.y < World::dim.y ){

        Plant nplant(npos, World::dim);
        glm::vec3 n = World::normal(nplant.index);

        if( World::discharge[nplant.index] < 0.2 &&
            n.y > 0.8 &&
            (float)(rand()%1000)/1000.0 > density[nplant.index]){
              nplant.root(density, World::dim, 1.0);
              plants.push_back(nplant);
            }
      }
    }

    //If the tree is in a pool or in a stream, kill it
    if(World::discharge[plants[i].index] > 0.5 ||
       rand()%1000 == 0 ){ //Random Death Chance
         plants[i].root(density, World::dim, -1.0);
         plants.erase(plants.begin()+i);
         i--;
       }
  }

  return true;

};



#endif
