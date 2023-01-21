#ifndef SIMPLEHYDROLOGY_CELLPOOL
#define SIMPLEHYDROLOGY_CELLPOOL

/*
  New World Data-Storage Data Structure:

  Either we do sort objects using an interleaved or a non-interleaved property buffer.
  Interleaved is of course easier.

  Then I have a set of map-sections, which index the pool.

  A droplet then operates in a specific map-section, which later can contain a stride.
  The world then simply represents the quadtree management interface for accessing
  various areas of the map.

  Once I can create a basic map, I need to render it.
  Then the erosion should also work directly.

  1. Mapcell contains base Structure
  2. A cellpool lets me return a cell pool indexing struct,
    which is the base element used by the quadtree
    That's basically it.
*/

/*
================================================================================
                              Raw Cell Pool Storage
================================================================================
*/

struct MapCell {

  float height;
  float discharge;
  float momentumx;
  float momentumy;

  float discharge_track;
  float momentumx_track;
  float momentumy_track;

};

template<typename T>
struct MapSection {

  T* start;
  size_t size;

};

template<typename T>
struct CellPool {

  T* start = NULL;
  size_t size = 0;

  deque<MapSection<T>> free;

  CellPool(){}
  CellPool(size_t _size){
    reserve(_size);
  }

  ~CellPool(){
    if(start != NULL)
      delete[] start;
  }

  void reserve(size_t _size){
    size = _size;
    start = new T[size];
    free.emplace_front(start, size);
  }

  MapSection<T> get(size_t _size){

    if(free.empty())
      return {NULL, 0};

    if(_size > size)
      return {NULL, 0};

    if(free.front().size < _size)
      return {NULL, 0};

    MapSection<MapCell> sec = {free.front().start, _size};
    free.front().start += _size;
    free.front().size -= _size;

    return sec;

  }

};

/*
================================================================================
                          Spatially Defined Storage
================================================================================
*/

struct MapIndex {

  ivec2 pos = ivec2(0);
  ivec2 dim = ivec2(0);

  MapSection<MapCell> cell;
  uint* section = NULL;

  MapCell& get(ivec2 p){
    return *(cell.start + math::flatten(p-pos, dim));
  }

  ivec2 randpos(){
      return pos + ivec2(rand()%dim.x, rand()%dim.y);
  }

  inline bool oob(ivec2 p){
    if(p.x >= pos.x + dim.x) return true;
    if(p.y >= pos.y + dim.y) return true;
    if(p.x < pos.x) return true;
    if(p.y < pos.y) return true;
    return false;
  }

  glm::vec3 normal(vec2 pos);

};

glm::vec3 MapIndex::normal(vec2 pos){

  glm::vec3 n = glm::vec3(0);

  //Two large triangels adjacent to the plane (+Y -> +X) (-Y -> -X)
  if(!oob(pos+glm::vec2( 1, 1)))
    n += glm::cross(glm::vec3( 0.0, SCALE*(get(pos+glm::vec2(0,1)).height - get(pos).height), 1.0), glm::vec3( 1.0, SCALE*(get(pos+glm::vec2(1,0)).height - get(pos).height), 0.0));

  if(!oob(pos+glm::vec2(-1,-1)))
    n += glm::cross(glm::vec3( 0.0, SCALE*(get(pos-glm::vec2(0,1)).height - get(pos).height),-1.0), glm::vec3(-1.0, SCALE*(get(pos-glm::vec2(1,0)).height - get(pos).height), 0.0));

  //Two Alternative Planes (+X -> -Y) (-X -> +Y)
  if(!oob(pos+glm::vec2( 1,-1)))
    n += glm::cross(glm::vec3( 1.0, SCALE*(get(pos+glm::vec2(1,0)).height - get(pos).height), 0.0), glm::vec3( 0.0, SCALE*(get(pos-glm::vec2(0,1)).height - get(pos).height),-1.0));

  if(!oob(pos+glm::vec2(-1, 1)))
    n += glm::cross(glm::vec3(-1.0, SCALE*(get(pos-glm::vec2(1,0)).height - get(pos).height), 0.0), glm::vec3( 0.0, SCALE*(get(pos+glm::vec2(0,1)).height - get(pos).height), 1.0));

  return glm::normalize(n);

}

void indexmap(Vertexpool<Vertex>& vertexpool, MapIndex& index){

//  int start = vertexpool.indices.size();

  size_t size = 0;

  for(int i = 0; i < index.dim.x-1; i++){
  for(int j = 0; j < index.dim.y-1; j++){

    vertexpool.indices.push_back(math::flatten(ivec2(i, j), index.dim));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), index.dim));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), index.dim));

    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j), index.dim));
    vertexpool.indices.push_back(math::flatten(ivec2(i, j+1), index.dim));
    vertexpool.indices.push_back(math::flatten(ivec2(i+1, j+1), index.dim));

    size += 6;

  }}

  vertexpool.resize(index.section, vertexpool.indices.size());
  vertexpool.index();
  vertexpool.update();

}

//Use the Position Hash (Generates Hash from Index)
std::hash<std::string> position_hash;
double hashrand(int i){
  return (double)(position_hash(std::to_string(i))%1000)/1000.0;
}

void updatemap(Vertexpool<Vertex>& vertexpool, MapIndex& index){

  for(int i = index.pos.x; i < index.pos.x + index.dim.x; i++)
  for(int j = index.pos.y; j < index.pos.y + index.dim.y; j++){

    float hash = hashrand(math::flatten(ivec2(i, j), index.dim));
    float p = erf(0.4f*index.get(vec2(i, j)).discharge);

    float height = SCALE*index.get(ivec2(i, j)).height;
    glm::vec3 color = flatColor;

    glm::vec3 normal = index.normal(vec2(i, j));
    if(normal.y < steepness)
      color = steepColor;

    color = glm::mix(color, waterColor, p);

    color = glm::mix(color, vec3(0), 0.3*hash*(1.0f-p));

    vertexpool.fill(index.section, math::flatten(ivec2(i, j)-index.pos, index.dim),
      glm::vec3(i, height, j),
      normal,
      vec4(color, 1.0f)
    );

  }

}

#endif
