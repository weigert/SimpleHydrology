#ifndef SIMPLEHYDROLOGY_CELLPOOL
#define SIMPLEHYDROLOGY_CELLPOOL

/*
================================================================================
                    Interleaved Cell Data Memory Pool
================================================================================
  Individual cell properties are stored in an interleaved data format.
  The mappool acts as a fixed-size memory pool for these cells.
  This acts as the base for creating sliceable, indexable, iterable map regions.
*/

namespace mappool {

// Raw Interleaved Data Buffer
template<typename T>
struct buf {

  T* start = NULL;
  size_t size = 0;

};

// Raw Interleaved Data Buffer Slice
template<typename T>
struct slice {

  mappool::buf<T> root;
  ivec2 res = ivec2(0);
  int scale = 1;

  const inline size_t size(){
    return res.x * res.y;
  }

  const inline bool oob(const ivec2 p){
    if(p.x >= res.x)  return true;
    if(p.y >= res.y)  return true;
    if(p.x  < 0)      return true;
    if(p.y  < 0)      return true;
    return false;
  }

  inline T* get(const ivec2 p){
    if(root.start == NULL) return NULL;
    if(oob(p)) return NULL;
    return root.start + math::flatten(p, res);
  }

};

// Raw Interleaved Data Pool
template<typename T>
struct pool {

  buf<T> root;
  deque<buf<T>> free;

  pool(){}
  pool(size_t _size){
    reserve(_size);
  }

  ~pool(){
    if(root.start != NULL){
      delete[] root.start;
      root.start = NULL;
    }
  }

  void reserve(size_t _size){
    root.size = _size;
    root.start = new T[root.size];
    free.emplace_front(root.start, root.size);
  }

  buf<T> get(size_t _size){

    if(free.empty())
      return {NULL, 0};

    if(_size > root.size)
      return {NULL, 0};

    if(free.front().size < _size)
      return {NULL, 0};

    buf<T> sec = {free.front().start, _size};
    free.front().start += _size;
    free.front().size -= _size;

    return sec;

  }

};

};  // namespace mappool

/*
================================================================================
                Cell Buffer Spatial Indexing / Slicing
================================================================================
  A mapslice acts as an indexable, bound-checking structure for this.
  This is the base-structure for retrieving data.
*/

namespace quadmap {

template<typename T>
struct node {

  ivec2 pos = ivec2(0); // Absolute World Position
  ivec2 res = ivec2(0); // Absolute Resolution

  uint* vertex = NULL;  // Vertexpool Rendering Pointer
  mappool::slice<T> s;  // Raw Interleaved Data Slices

  inline T* get(const ivec2 p){
    return s.get((p - pos)/s.scale);
  }

  const inline bool oob(const ivec2 p){
    return s.oob((p - pos)/s.scale);
  }

};

template<int N, typename T>
void indexnode(Vertexpool<Vertex>& vertexpool, node<T>& t){

  for(int i = 0; i < (t.res.x)/N-1; i++){
  for(int j = 0; j < (t.res.y)/N-1; j++){

    vertexpool.indices.push_back(math::flatten(ivec2(i, j), t.res/N));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), t.res/N));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), t.res/N));

    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), t.res/N));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), t.res/N));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j+1), t.res/N));

  }}

  vertexpool.resize(t.vertex, vertexpool.indices.size());
  vertexpool.index();
  vertexpool.update();

}


















/*



*/

template<typename T>
struct map {

  vector<node<T>> nodes;

  const inline bool oob(ivec2 p){
    for(auto& node: nodes)
    if(!node.oob(p))
      return false;
    return true;
  }

  inline node<T>* get(const ivec2 p){
    for(auto& node: nodes)
    if(!node.oob(p))
      return &node;
    return NULL;
  }

};

}; // namespace quadmap

/*
================================================================================
                Concrete Interleaved Cell Data Implementation
================================================================================
  A mapslice acts as an indexable, bound-checking structure for this.
  This is the base-structure for retrieving data.

  We want a hierarchy of MapCellSegments.
  They should have different size.
  I should be able to operate on any specific layer.
  This means each layer needs its own OOB check and stuff


    A number of templated reduction functions are supplied for extracting data
    as desired. This way we can define reductions for different combinations
    of slices, e.g. stacked slices.

*/

namespace reduce {
using namespace glm;

// Raw Interleaved Cell Data
struct cell {

  float height;
  float discharge;
  float momentumx;
  float momentumy;

  float discharge_track;
  float momentumx_track;
  float momentumy_track;

};

/*
    Reduction Functions!
*/

inline float height(quadmap::node<cell>& t, ivec2 p){
  return t.get(p)->height;
}

inline float height(quadmap::map<cell>& t, ivec2 p){
  for(auto& node: t.nodes)
  if(!node.oob(p))
    return height(node, p);
  return 0.0f;
}

inline float discharge(quadmap::node<cell>* t, ivec2 p){
  return erf(0.4f*t->get(p)->discharge);
}

inline float discharge(quadmap::map<cell>& t, ivec2 p){
  for(auto& node: t.nodes)
  if(!node.oob(p))
    return discharge(&node, p);
  return 0.0f;
}


template<typename T>
vec3 normal(T& t, ivec2 p){

  vec3 n = vec3(0, 0, 0);
  const vec3 s = vec3(1.0, SCALE, 1.0);

  if(!t.oob(p + ivec2( 1, 1)))
    n += cross( s*vec3( 0.0, height(t, p+ivec2( 0, 1)) - height(t, p), 1.0), s*vec3( 1.0, height(t, p+ivec2( 1, 0)) - height(t, p), 0.0));

  if(!t.oob(p + ivec2(-1,-1)))
    n += cross( s*vec3( 0.0, height(t, p-ivec2( 0, 1)) - height(t, p),-1.0), s*vec3(-1.0, height(t, p-ivec2( 1, 0)) - height(t, p), 0.0));

  //Two Alternative Planes (+X -> -Y) (-X -> +Y)
  if(!t.oob(p + ivec2( 1,-1)))
    n += cross( s*vec3( 1.0, height(t, p+ivec2( 1, 0)) - height(t, p), 0.0), s*vec3( 0.0, height(t, p-ivec2( 0, 1)) - height(t, p),-1.0));

  if(!t.oob(p + ivec2(-1, 1)))
    n += cross( s*vec3(-1.0, height(t, p-ivec2( 1, 0)) - height(t, p), 0.0), s*vec3( 0.0, height(t, p+ivec2( 0, 1)) - height(t, p), 1.0));

  if(length(n) > 0)
    n = normalize(n);
  return n;

}





};

template<int N, typename T>
void updatenode(Vertexpool<Vertex>& vertexpool, quadmap::node<T>& t){

  for(int i = t.pos.x; i < t.pos.x + t.res.x/N; i++)
  for(int j = t.pos.y; j < t.pos.y + t.res.y/N; j++){

    float hash = 0.0f;//hashrand(math::flatten(ivec2(i, j), t.res));
    float p = reduce::discharge(&t, N*ivec2(i, j));

    float height = SCALE*reduce::height(t, N*ivec2(i, j));
    glm::vec3 color = flatColor;

    glm::vec3 normal = reduce::normal(t, N*ivec2(i, j));
    if(normal.y < steepness)
      color = steepColor;

    color = glm::mix(color, waterColor, p);

    color = glm::mix(color, vec3(0), 0.3*hash*(1.0f-p));

    vertexpool.fill(t.vertex, math::flatten(ivec2(i, j)-t.pos, t.res/N),
      glm::vec3(N*i, height, N*j),
      normal,
      vec4(color, 1.0f)
    );

  }

}






















#endif
