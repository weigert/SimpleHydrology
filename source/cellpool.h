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

// Interleaved Data Slicing Structure
template<typename T>
struct slice {

  mappool::buf<T> root;
  ivec2 res = ivec2(0);

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

template<typename T>
struct index {

  ivec2 pos = ivec2(0); // Absolute World Position
  ivec2 res = ivec2(0); // Absolute Resolution

  slice<T> s;           // Raw Interleaved Data Slice
  uint* vertex = NULL;  // Vertexpool Rendering Pointer

  inline T* get(const ivec2 p){
    return s.get(p - pos);
  }

  const inline bool oob(const ivec2 p){
    return s.oob(p - pos);
  }

};

template<typename T>
struct map {

  vector<index<T>> indices;

  inline T* get(const ivec2 p){
    for(auto& index: indices)
    if(!index.oob(p))
      return index.get(p);
    return NULL;
  }

  const inline bool oob(ivec2 p){
    for(auto& index: indices)
    if(!index.oob(p))
      return false;
    return true;
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

template<typename T>
inline float height(T& t, vec2 p){
  return t.get(p)->height;
}

template<typename T>
inline float discharge(T& t, vec2 p){
  return erf(0.4f*t.get(p)->discharge);
}

template<typename T>
vec3 normal(T& t, vec2 p){

  vec3 n = vec3(0, 0, 0);
  const vec3 s = vec3(1.0, SCALE, 1.0);

  if(!t.oob(p + vec2( 1, 1)))
    n += cross( s*vec3( 0.0, height(t, p+vec2( 0, 1)) - height(t, p), 1.0), s*vec3( 1.0, height(t, p+vec2( 1, 0)) - height(t, p), 0.0));

  if(!t.oob(p + vec2(-1,-1)))
    n += cross( s*vec3( 0.0, height(t, p-vec2( 0, 1)) - height(t, p),-1.0), s*vec3(-1.0, height(t, p-vec2( 1, 0)) - height(t, p), 0.0));

  //Two Alternative Planes (+X -> -Y) (-X -> +Y)
  if(!t.oob(p + vec2( 1,-1)))
    n += cross( s*vec3( 1.0, height(t, p+vec2( 1, 0)) - height(t, p), 0.0), s*vec3( 0.0, height(t, p-vec2( 0, 1)) - height(t, p),-1.0));

  if(!t.oob(p + vec2(-1, 1)))
    n += cross( s*vec3(-1.0, height(t, p-vec2( 1, 0)) - height(t, p), 0.0), s*vec3( 0.0, height(t, p+vec2( 0, 1)) - height(t, p), 1.0));

  if(length(n) > 0)
    n = normalize(n);
  return n;

}

};








template<typename T>
void indexmap(Vertexpool<Vertex>& vertexpool, T& t){

  for(int i = 0; i < t.res.x-1; i++){
  for(int j = 0; j < t.res.y-1; j++){

    vertexpool.indices.push_back(math::flatten(ivec2(i, j), t.res));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), t.res));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), t.res));

    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), t.res));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), t.res));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j+1), t.res));

  }}

  vertexpool.resize(t.vertex, vertexpool.indices.size());
  vertexpool.index();
  vertexpool.update();

}

//Use the Position Hash (Generates Hash from Index)
std::hash<std::string> position_hash;
double hashrand(int i){
  return (double)(position_hash(std::to_string(i))%1000)/1000.0;
}

template<typename T>
void updatemap(Vertexpool<Vertex>& vertexpool, T& t){

  for(int i = t.pos.x; i < t.pos.x + t.res.x; i++)
  for(int j = t.pos.y; j < t.pos.y + t.res.y; j++){

    float hash = hashrand(math::flatten(ivec2(i, j), t.res));
    float p = erf(0.4f*t.get(vec2(i, j))->discharge);

    float height = SCALE*reduce::height(t, vec2(i, j));
    glm::vec3 color = flatColor;

    glm::vec3 normal = reduce::normal(t, vec2(i, j));
    if(normal.y < steepness)
      color = steepColor;

    color = glm::mix(color, waterColor, p);

    color = glm::mix(color, vec3(0), 0.3*hash*(1.0f-p));

    vertexpool.fill(t.vertex, math::flatten(ivec2(i, j)-t.pos, t.res),
      glm::vec3(i, height, j),
      normal,
      vec4(color, 1.0f)
    );

  }

}















#endif
