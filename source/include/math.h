#ifndef SIMPLEHYDROLOGY_MATH
#define SIMPLEHYDROLOGY_MATH

namespace math {

  inline int flatten(ivec2 pos, const ivec2 dim){
    return pos.x * dim.y + pos.y;
  }

}



#endif
