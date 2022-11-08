struct Plant{
  Plant(int i, glm::ivec2 d){
    index = i;
    pos = math::cunflatten(i, d);
  };

  Plant(glm::ivec2 p, glm::ivec2 d){
    pos = p;
    index = math::cflatten(pos, d);
  };

  glm::ivec2 pos;
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
  density[index] += f*1.0;

  if(pos.x > 0){

      density[math::cflatten(pos + ivec2(-1, 0), dim)] += f*0.6;    //(-1, 0)

    if(pos.y > 0)
      density[math::cflatten(pos + ivec2(-1,-1), dim)] += f*0.4;    //(-1, -1)

    if(pos.y < WSIZE-1)
      density[math::cflatten(pos + ivec2(-1, 1), dim)] += f*0.4;    //(-1, 1)

  }

  if(pos.x < WSIZE-1){

      density[math::cflatten(pos + ivec2( 1, 0), dim)] += f*0.6;    //(1, 0)

    if(pos.y > 0)
      density[math::cflatten(pos + ivec2( 1,-1), dim)] += f*0.4;    //(1, -1)

    if(pos.y < WSIZE-1)
      density[math::cflatten(pos + ivec2( 1, 1), dim)] += f*0.4;    //(1, 1)

  }

  if(pos.y > 0)
    density[math::cflatten(pos + ivec2( 0,-1), dim)]   += f*0.6;    //(0, -1)

  if(pos.y < WSIZE-1)
    density[math::cflatten(pos + ivec2( 0, 1), dim)]   += f*0.6;    //(0, 1)

}
