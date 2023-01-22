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

  For now, our map implementation uses a fixed arrangement of tiles.
  Soon, we will be able to switch to a quadtree with arbitrary shape.

  Finally, we can make the nodes have multiple scales and reimplement
  their retrieval functions.
*/

namespace quad {

const int mapscale = 80;

const int tilesize = 512;
const int tilearea = tilesize*tilesize;
const ivec2 tileres = ivec2(tilesize);

const int mapsize = 1;
const int maparea = mapsize*mapsize;

const int size = mapsize*tilesize;
const int area = maparea*tilearea;
const ivec2 res = ivec2(size);

const int lodsize = 1;
const int lodarea = lodsize*lodsize;

template<typename T>
vec3 _normal(T& t, ivec2 p){

  vec3 n = vec3(0, 0, 0);
  const vec3 s = vec3(1.0, quad::mapscale, 1.0);

  if(!t.oob(p + quad::lodsize*ivec2( 1, 1)))
    n += cross( s*vec3( 0.0, t.height(p+quad::lodsize*ivec2( 0, 1)) - t.height(p), 1.0), s*vec3( 1.0, t.height(p+quad::lodsize*ivec2( 1, 0)) - t.height(p), 0.0));

  if(!t.oob(p + quad::lodsize*ivec2(-1,-1)))
    n += cross( s*vec3( 0.0, t.height(p-quad::lodsize*ivec2( 0, 1)) - t.height(p),-1.0), s*vec3(-1.0, t.height(p-quad::lodsize*ivec2( 1, 0)) - t.height(p), 0.0));

  //Two Alternative Planes (+X -> -Y) (-X -> +Y)
  if(!t.oob(p + quad::lodsize*ivec2( 1,-1)))
    n += cross( s*vec3( 1.0, t.height(p+quad::lodsize*ivec2( 1, 0)) - t.height(p), 0.0), s*vec3( 0.0, t.height(p-quad::lodsize*ivec2( 0, 1)) - t.height(p),-1.0));

  if(!t.oob(p + quad::lodsize*ivec2(-1, 1)))
    n += cross( s*vec3(-1.0, t.height(p-quad::lodsize*ivec2( 1, 0)) - t.height(p), 0.0), s*vec3( 0.0, t.height(p+quad::lodsize*ivec2( 0, 1)) - t.height(p), 1.0));

  if(length(n) > 0)
    n = normalize(n);
  return n;

}

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

struct node {

  ivec2 pos = ivec2(0);   // Absolute World Position
  uint* vertex = NULL;    // Vertexpool Rendering Pointer
  mappool::slice<cell> s; // Raw Interleaved Data Slices

  inline cell* get(const ivec2 p){
    return s.get((p - pos)/lodsize);
  }

  const inline bool oob(const ivec2 p){
    return s.oob((p - pos)/lodsize);
  }

  const inline float height(ivec2 p){
    return get(p)->height;
  }

  const inline float discharge(ivec2 p){
    return erf(0.4f*get(p)->discharge);
  }

  const inline vec3 normal(ivec2 p){
    return _normal(*this, p);
  }

};

void indexnode(Vertexpool<Vertex>& vertexpool, quad::node& t){

  for(int i = 0; i < tilesize/lodsize-1; i++){
  for(int j = 0; j < tilesize/lodsize-1; j++){

    vertexpool.indices.push_back(math::flatten(ivec2(i, j), tileres/lodsize));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), tileres/lodsize));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), tileres/lodsize));

    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), tileres/lodsize));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), tileres/lodsize));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j+1), tileres/lodsize));

  }}

  vertexpool.resize(t.vertex, vertexpool.indices.size());
  vertexpool.index();
  vertexpool.update();

}

void updatenode(Vertexpool<Vertex>& vertexpool, quad::node& t){

  for(int i = 0; i < tilesize/lodsize; i++)
  for(int j = 0; j < tilesize/lodsize; j++){

    float hash = 0.0f;
    float p = t.discharge(t.pos + lodsize*ivec2(i, j));

    float height = quad::mapscale*t.height(t.pos + lodsize*ivec2(i, j));
    glm::vec3 color = flatColor;

    glm::vec3 normal = t.normal(t.pos + lodsize*ivec2(i, j));
    if(normal.y < steepness)
      color = steepColor;

    color = glm::mix(color, waterColor, p);

    color = glm::mix(color, vec3(0), 0.3*hash*(1.0f-p));

    vertexpool.fill(t.vertex, math::flatten(ivec2(i, j), tileres/lodsize),
      glm::vec3(t.pos.x + lodsize*i, height, t.pos.y + lodsize*j),
      normal,
      vec4(color, 1.0f)
    );

  }

}

struct map {

  node nodes[maparea];

  void init(Vertexpool<Vertex>& vertexpool, mappool::pool<cell>& cellpool, int SEED){

    // Generate the Node Array

    for(int i = 0; i < mapsize; i++)
    for(int j = 0; j < mapsize; j++){

      int ind = i*mapsize + j;

      nodes[ind] = {
        tileres*ivec2(i, j),
        vertexpool.section(tilearea/lodarea, 0, glm::vec3(0), vertexpool.indices.size()),
        { cellpool.get(tilearea/lodarea), tileres/lodsize }
      };

      indexnode(vertexpool, nodes[ind]);

    }

    // Fill the Node Array

    std::cout<<"Generating New World"<<std::endl;
    std::cout<<"Seed: "<<SEED<<std::endl;

    std::cout<<"... generating height ..."<<std::endl;

    static FastNoiseLite noise; //Noise System
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);

    float min, max = 0.0;

    for(auto& node: nodes){

      // Reset
      for(int i = 0; i <  tilesize; i++)
      for(int j = 0; j <  tilesize; j++)
        node.get(node.pos + ivec2(i, j))->height = 0.0f;

      // Add Layers of Noise

      float frequency = 1.0f;
      float scale = 0.6f;

      for(size_t o = 0; o < 8; o++){

        noise.SetFrequency(frequency);

        for(int i = 0; i <  tilesize; i++)
        for(int j = 0; j <  tilesize; j++){
          vec2 p = (vec2(node.pos) + vec2(i, j))/vec2(quad::tileres);
          node.get(node.pos + ivec2(i, j))->height += scale*noise.GetNoise(p.x, p.y, (float)(SEED%10000));
        }

        frequency *= 2;
        scale *= 0.6;

      }

    }

  }

  const inline bool oob(ivec2 p){
    if(p.x  < 0)  return true;
    if(p.y  < 0)  return true;
    if(p.x >= size)  return true;
    if(p.y >= size)  return true;
    return false;
  }

  inline node* get(ivec2 p){
    if(oob(p)) return NULL;
    p /= tileres;
    int ind = p.x*mapsize + p.y;
    return &nodes[ind];
  }

  const inline float height(ivec2 p){
    node* n = get(p);
    if(n == NULL) return 0.0f;
    return n->height(p);
  }

  const inline float discharge(ivec2 p){
    node* n = get(p);
    if(n == NULL) return 0.0f;
    return n->discharge(p);
  }

  const inline vec3 normal(ivec2 p){
    return _normal(*this, p);
  }

};

}; // namespace quad

#endif
