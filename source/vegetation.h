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
  void root(double* density, glm::ivec2 dim, double factor);

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

void Plant::root(double* density, glm::ivec2 dim, double f){

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
