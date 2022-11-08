/*
================================================================================
              SimpleHydrology Export to Uncompressed Voxel Format
================================================================================
*/

namespace voxel {

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;

#include <glm/glm.hpp>
using namespace glm;
using namespace std;

const unsigned int CHUNKSIZE = 16;

const ivec3 WDIM = ivec3(WSIZE/CHUNKSIZE, SCALE/CHUNKSIZE+1, WSIZE/CHUNKSIZE); // Full-World Dimensions (in chunks)
const ivec3 RDIM = ivec3(16, 16, 16); // Sub-Region Size (in chunks)
const ivec3 CDIM = ivec3(CHUNKSIZE);  // Chunk Size as Vector
const ivec3 SDIM = ivec3(4, 4, 4);    // RESCALE DIM

const unsigned int CVOL = CDIM.x*CDIM.y*CDIM.z;
const unsigned int RVOL = RDIM.x*RDIM.y*RDIM.z;
const unsigned int WVOL = WDIM.x*WDIM.y*WDIM.z;

// Block Types Enumerator

enum block: unsigned char {

  BLOCK_AIR,
  BLOCK_GRASS,
  BLOCK_DIRT,
  BLOCK_SAND,
  BLOCK_CLAY,
  BLOCK_GRAVEL,
  BLOCK_SANDSTONE,
  BLOCK_STONE,
  BLOCK_WATER,
  BLOCK_LEAVES,
  BLOCK_WOOD,
  BLOCK_PUMPKIN,
  BLOCK_CACTUS,
  BLOCK_PLANKS,
  BLOCK_GLASS,
  BLOCK_VOID,
  BLOCK_CACTUSFLOWER

};

// Blueprint Class for buffered In-Order Writing

struct bufferObject {

  bufferObject(){}

  bufferObject(ivec3 p, block t){
    set(p, t);
  }
  void set(ivec3 p, block t){
    pos = p; type = t;
    cpos = pos/CDIM;
    rpos = cpos/RDIM;
  }

  ivec3 pos = ivec3(0);        //Block Position in Chunk
  ivec3 cpos = ivec3(0);       //Block Position in Chunk
  ivec3 rpos = ivec3(0);       //Chunk Position in Region
  block type = BLOCK_AIR; //Metadata

};

bool operator > (const bufferObject& a, const bufferObject& b) {

  if(a.rpos.x > b.rpos.x) return true;    //Sort by Region (3D)
  if(a.rpos.x < b.rpos.x) return false;

  if(a.rpos.y > b.rpos.y) return true;
  if(a.rpos.y < b.rpos.y) return false;

  if(a.rpos.z > b.rpos.z) return true;
  if(a.rpos.z < b.rpos.z) return false;

  if(a.cpos.x > b.cpos.x) return true;    //Sort by Chunk (3D)
  if(a.cpos.x < b.cpos.x) return false;

  if(a.cpos.y > b.cpos.y) return true;
  if(a.cpos.y < b.cpos.y) return false;

  if(a.cpos.z > b.cpos.z) return true;
  if(a.cpos.z < b.cpos.z) return false;

  return false;

}

// Blueprint Class

class Blueprint {
public:

  vector<bufferObject> edits;

  bool add(ivec3, block, bool);   //Add Block
  void clean(bool);               //Remove Duplicates

  Blueprint& operator += (Blueprint& lhs){              //Merge Blueprints
    for(bufferObject& obj : lhs.edits)
      this->edits.push_back(obj);
    return *this;
  }

  Blueprint& operator + (Blueprint& lhs){
    for(bufferObject& obj : lhs.edits)
      this->edits.push_back(obj);
    return *this;
  }

  Blueprint& operator + (const ivec3 shift){                  //Shift Blueprints
    size_t n = this->edits.size();
    while(n > 0){
      bufferObject& obj = this->edits[0];
      this->add(obj.pos + shift, obj.type, false);
      this->edits.erase(this->edits.begin(), this->edits.begin()+1);
      n--;
    }
    return *this;
  }

  bool write(string);

};

bool Blueprint::add(ivec3 pos, block type, bool negative = false){

  if(!negative && (any(lessThan(pos, ivec3(0)))          //Check Boundary Conditions
  || any(greaterThanEqual(pos, CDIM*WDIM*SDIM))))
    return false;

  edits.emplace_back(pos, type);  //Add to Edit Buffer
  return true;

}

void Blueprint::clean(bool later = true){

  Blueprint newprint;

  for(unsigned int i = 0; i < edits.size(); i++){

    bufferObject* buf;
    if(!later) buf = &edits[i];
    else buf = &edits[edits.size()-1 - i];

    bool duplicate = false;

    for(unsigned int j = 0; j < newprint.edits.size(); j++){
      if(!all(equal(buf->pos+buf->cpos*CDIM, newprint.edits[j].pos+newprint.edits[j].cpos*CDIM)))
        continue;
      duplicate = true;
      break;
    }

    if(!duplicate)  //Add First Occurence
      newprint.edits.push_back(edits[i]);

  }

  *this = newprint;

}

/*
================================================================================
                         Actual Chunk (Has a Format)
================================================================================
*/

class Chunk{
public:

  ivec3 pos = ivec3(0);
  block* data = NULL;

  Chunk(block type = BLOCK_AIR){
    data = new block[CVOL]{type};
  }

  Chunk(ivec3 p):Chunk(){
    pos = p;
  }

  ~Chunk(){
    if(data != NULL)
      delete[] data;
  }

  int index(ivec3 p){
    if(all(lessThan(p, CDIM)))
      return p.x*CHUNKSIZE*CHUNKSIZE+p.y*CHUNKSIZE+p.z;
    return 0;
  }

  void set(ivec3 p, block type){
    data[index(p)] = type;
  }

  block get(ivec3 p){
    return data[index(p)];
  }

  block get(int* p){
    return data[p[0]*CHUNKSIZE*CHUNKSIZE+p[1]*CHUNKSIZE+p[2]];
  }

};

bool operator > (const Chunk& a, const Chunk& b) {

  if(a.pos.x > b.pos.x) return true;    //Sort by Position
  if(a.pos.x < b.pos.x) return false;

  if(a.pos.y > b.pos.y) return true;    //Sort by Position
  if(a.pos.y < b.pos.y) return false;

  if(a.pos.z > b.pos.z) return true;    //Sort by Position
  if(a.pos.z < b.pos.z) return false;

  return false;

}

bool operator < (const Chunk& a, const Chunk& b) {

  if(a.pos.x < b.pos.x) return true;    //Sort by Position
  if(a.pos.x > b.pos.x) return false;

  if(a.pos.y < b.pos.y) return true;    //Sort by Position
  if(a.pos.y > b.pos.y) return false;

  if(a.pos.z < b.pos.z) return true;    //Sort by Position
  if(a.pos.z > b.pos.z) return false;

  return false;

}

/*
================================================================================
                   EXPORT A VOXEL MAP FROM A BLUEPRINT
================================================================================
*/

string tostring(ivec3 v){
  stringstream ss;
  ss << v.x << v.y << v.z;
  return ss.str();
}

void blank(string savename){

  // Save Directories
  fs::path rootdir = fs::path(fs::current_path() / "save");
  if(!fs::exists(rootdir))
    fs::create_directory(rootdir);

  // Save Directory
  fs::path savedir = rootdir/savename; //Overwrite I guess

  if(fs::exists(savedir))
    fs::remove_all(savedir);
  fs::create_directory(savedir);

  cout<<"Generating Blank Region Files..."<<endl;

  FILE* pFile = NULL;
  string oldfile = "";

  Chunk chunk;

  for(int i = 0; i < WDIM.x*SDIM.x; i++)
  for(int j = 0; j < WDIM.y*SDIM.y; j++)
  for(int k = 0; k < WDIM.z*SDIM.z; k++){

    string regionfile = "world.region"+tostring(ivec3(i, j, k)/RDIM);

    if(regionfile != oldfile){
      oldfile = regionfile;
      if(pFile != NULL){
         fclose(pFile);
         pFile = NULL;
      }

      pFile = fopen((savedir/regionfile).string().c_str(), "a+b");
      if(pFile == NULL){
        cout<<"Failed to open region file"<<endl;
        return;
      }

    }

    fwrite(chunk.data, sizeof(block), CVOL, pFile);

  }

  fclose(pFile);

}

bool Blueprint::write(string savename){

  fs::path rootdir = fs::path(fs::current_path() / "save");
  fs::path savedir = rootdir/savename; //Overwrite I guess

  //Evaluate Blueprint
  cout<<"Evaluating Blueprint ["<<edits.size()<<"]..."<<endl;

  if(edits.empty())
    return false;

  cout<<"Sorting buffer..."<<endl;
  sort(edits.begin(), edits.end(), greater<bufferObject>());

  Chunk chunk;

  cout<<"Writing to File..."<<endl;

  while(!edits.empty()){

    ivec3 rpos = edits.back().rpos;
    string regionfile = "world.region"+tostring(edits.back().rpos);

    FILE* inFile = fopen((savedir/regionfile).string().c_str(), "rb");
    if(inFile == NULL){
      cout<<"Failed to open region file "<<regionfile<<endl;
      return false;
    }

    FILE* outFile = fopen((savedir/(regionfile+".temp")).string().c_str(), "wb");
    if(inFile == NULL){
      cout<<"Failed to open region file "<<regionfile<<endl;
      return false;
    }

    for(int n = 0; n < RVOL; n++){

      if(fread(chunk.data, sizeof(block), CVOL, inFile) < CVOL)
        cout<<"Read Error"<<endl;

      function<ivec3(int, ivec3)> unflatten = [](int i, ivec3 s){
        int z = i%s.x;
        int y = (i/s.x)%s.y;
        int x = i/(s.x*s.y);
        return ivec3(x, y, z);
      };

      chunk.pos = unflatten(n, RDIM) + edits.back().rpos*RDIM;

      while(!edits.empty() && all(equal(chunk.pos, edits.back().cpos)) && all(equal(rpos, edits.back().rpos))){
        chunk.set(mod((vec3)edits.back().pos, (vec3)CDIM), edits.back().type);
        edits.pop_back();
      }

      if(fwrite(chunk.data, sizeof(block), CVOL, outFile) < CVOL)
        cout<<"Write Error"<<endl;

    }

    fclose(inFile);
    fclose(outFile);

    fs::remove_all(savedir/regionfile);
    fs::rename((savedir/(regionfile+".temp")), savedir/regionfile);

  }

  cout<<"Done."<<endl;

  return true;

}

Blueprint tree(int height){
  Blueprint print;

  for(int i = 0; i < height; i++){
    print.add(glm::vec3(0, i, 0), BLOCK_WOOD, true);
  }

  //Add Leaf-Crown
  print.add(glm::vec3(0, height+1, 0), BLOCK_LEAVES, true);
  print.add(glm::vec3(1, height, 0), BLOCK_LEAVES, true);
  print.add(glm::vec3(0, height, 1), BLOCK_LEAVES, true);
  print.add(glm::vec3(-1, height, 0), BLOCK_LEAVES, true);
  print.add(glm::vec3(0, height, -1), BLOCK_LEAVES, true);

  return print;
}

void ize(World& world, string savename){

  voxel::Blueprint blueprint;

  voxel::blank(savename);

  fs::path rootdir = fs::path(fs::current_path() / "save");
  fs::path savedir = rootdir/savename; //Overwrite I guess

  cout<<"Generating Meta File..."<<endl;

  std::fstream metafile((savedir/"world.meta").string().c_str(), std::ios::out);
  if(!metafile.is_open()){
    cout<<"Failed to open world meta file"<<endl;
    return;
  }

  metafile<<"# territory world meta file"<<endl;
  metafile<<"WORLD "<<savename<<endl;
  metafile<<"SEED "<<world.SEED<<endl;
  metafile<<"AGE "<<world.AGE<<endl;
  metafile<<"WDIM "<<WDIM.x*SDIM.x<<" "<<WDIM.y*SDIM.y<<" "<<WDIM.z*SDIM.z<<endl;
  metafile<<"RDIM "<<RDIM.x<<" "<<RDIM.y<<" "<<RDIM.z<<endl;
  metafile<<"CDIM "<<CDIM.x<<" "<<CDIM.y<<" "<<CDIM.z<<endl;
  metafile<<"CODEC RAW"<<endl;

  // I should add the meta file here...

  const std::function<float(int, int, int, int, float*)> lerp = [&](int x, int z, int sx, int sz, float* m){

    float mx = (float)sx/(float)SDIM.x;
    float mz = (float)sz/(float)SDIM.z;

    float ml = 0.0f;

    if(x < world.dim.x && z < world.dim.y){
      ml += (1.0f-mx)*(1.0f-mz)*m[x*world.dim.x+z];
      ml += mx*(1.0f-mz)*m[(x+1)*world.dim.x+z];
      ml += (1.0f-mx)*mz*m[x*world.dim.x+z+1];
      ml += mx*mz*m[(x+1)*world.dim.x+z+1];
    }
    else ml = m[x*world.dim.x+z];

    return ml;

  };

  cout<<"Voxelizing Height Data..."<<endl;

  for(int x = 0; x < world.dim.x; x++)
  for(int z = 0; z < world.dim.y; z++){

    for(int sx = 0; sx < SDIM.x; sx++)
    for(int sz = 0; sz < SDIM.z; sz++){

      int fx = x*SDIM.x + sx;
      int fz = z*SDIM.z + sz;

      float h = SCALE*SDIM.y*lerp(x, z, sx, sz, world.heightmap);

      vec3 n = world.normal(x*world.dim.x + z);
      bool rock = (n.y < 0.8);

      for(int y = 0; y < h-3; y++)
        blueprint.add(ivec3(fx, y, fz), voxel::BLOCK_STONE);

      if(n.y < 0.8){
        blueprint.add(ivec3(fx, h-1, fz), voxel::BLOCK_STONE);
        blueprint.add(ivec3(fx, h-2, fz), voxel::BLOCK_STONE);
        blueprint.add(ivec3(fx, h-3, fz), voxel::BLOCK_STONE);
      }
      else{
        blueprint.add(ivec3(fx, h-1, fz), voxel::BLOCK_GRASS);
        blueprint.add(ivec3(fx, h-2, fz), voxel::BLOCK_GRASS);
        blueprint.add(ivec3(fx, h-3, fz), voxel::BLOCK_GRASS);
      }

    }

    if(blueprint.edits.size() > (2 << 24))
      blueprint.write(savename);

  }

  blueprint.write(savename);

  cout<<"Voxelizing Stream Data..."<<endl;

  for(int x = 0; x < world.dim.x; x++)
  for(int z = 0; z < world.dim.y; z++){

    for(int sx = 0; sx < SDIM.x; sx++)
    for(int sz = 0; sz < SDIM.z; sz++){

      int fx = x*SDIM.x + sx;
      int fz = z*SDIM.z + sz;

      float h = SDIM.y*SCALE*lerp(x, z, sx, sz, world.heightmap);
      float s = lerp(x, z, sx, sz, world.waterpath);

      blueprint.add(ivec3(fx, h, fz), voxel::BLOCK_AIR);
      if(s > 0.2) blueprint.add(ivec3(fx, h-1, fz), voxel::BLOCK_WATER);
      if(s > 0.3) blueprint.add(ivec3(fx, h-2, fz), voxel::BLOCK_WATER);
      if(s > 0.4) blueprint.add(ivec3(fx, h-3, fz), voxel::BLOCK_WATER);

    }

    if(blueprint.edits.size() > (2 << 24))
      blueprint.write(savename);

  }

  blueprint.write(savename);

  cout<<"Voxelizing Vegetation Data..."<<endl;

  //Add trees according to the tree number and density
  for(size_t n = 0; n < 16*world.trees.size(); n++){

    int p[2] = {rand()%(int)(world.dim.x*SDIM.x), rand()%(int)(world.dim.y*SDIM.z)};

    int x = p[0]/SDIM.x;
    int z = p[1]/SDIM.z;
    int sx = p[0]%SDIM.x;
    int sz = p[1]%SDIM.z;

    if((float)(rand()%1000)/1000.0 > world.plantdensity[x*world.dim.x+z])
      continue;

    float height = SDIM.y*SCALE*lerp(x, z, sx, sz, world.heightmap);
    blueprint += (voxel::tree(rand()%6+4) + vec3(p[0], (int)height, p[1]));

  }

  blueprint.write(savename);

}

};
