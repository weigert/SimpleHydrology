#include <TinyEngine/TinyEngine>
#include <TinyEngine/camera>
#include <TinyEngine/image>

#define WSIZE 512
#define FREQUENCY 1
#define SCALE 80
#define RES 16
#define TILES 4

#include "source/model.h"
#include "source/vertexpool.h"
#include "source/world.h"

mappool::pool<quad::cell> cellpool;
Vertexpool<Vertex> vertexpool;

int main( int argc, char* args[] ) {

  Tiny::view.vsync = false;
  Tiny::view.antialias = 0;
  Tiny::window("Simple Hydrology", WIDTH, HEIGHT);

  //Initialize the World

  cellpool.reserve(TILES*TILES*WSIZE*WSIZE);
  vertexpool.reserve(WSIZE*WSIZE, TILES*TILES);

  World world;

  for(int i = 0; i < TILES; i++)
  for(int j = 0; j < TILES; j++){

    world.map.add({
      ivec2(i*WSIZE, j*WSIZE),
      ivec2(WSIZE, WSIZE),
      vertexpool.section(WSIZE/RES*WSIZE/RES, 0, glm::vec3(0), vertexpool.indices.size())
    });

    world.map.nodes.back().s = {
      cellpool.get(WSIZE/RES*WSIZE/RES), ivec2(WSIZE, WSIZE)/RES, RES
    };

    indexnode<RES>(vertexpool, world.map.nodes.back());

  }












  if(argc >= 2){
    World::SEED = std::stoi(args[1]);
    srand(std::stoi(args[1]));
  }
  else {
    World::SEED = time(NULL);
    srand(World::SEED);
  }

  world.generate();

  //Vertexpool for Drawing Surface

  for(auto& node: world.map.nodes){
    updatenode<RES>(vertexpool, node);
  }











  //Initialize the Visualization


  glDisable(GL_CULL_FACE);

  cam::near = -800.0f;
  cam::far = 800.0f;
  cam::moverate = 10.0f;
  cam::look = glm::vec3(WSIZE/2, 0, WSIZE/2);
  cam::roty = 45.0f;
  cam::init(3, cam::ORTHO);
  cam::update();

  //Setup Shaders
  Shader shader({"source/shader/default.vs", "source/shader/default.fs"}, {"in_Position", "in_Normal", "in_Color"});
  Shader depth({"source/shader/depth.vs", "source/shader/depth.fs"}, {"in_Position"});
  Shader effect({"source/shader/effect.vs", "source/shader/effect.fs"}, {"in_Quad", "in_Tex"});
  Shader billboard({"source/shader/billboard.vs", "source/shader/billboard.fs"}, {"in_Quad", "in_Tex"});
  Shader sprite({"source/shader/sprite.vs", "source/shader/sprite.fs"}, {"in_Quad", "in_Tex", "in_Model"});
  Shader spritedepth({"source/shader/spritedepth.vs", "source/shader/spritedepth.fs"}, {"in_Quad", "in_Tex", "in_Model"});
  Shader heightshader({"source/shader/height.vs", "source/shader/height.fs"}, {"in_Position"});

  //Trees as a Particle System
  Square3D treemodel;									//Model we want to instance render!
  Instance treeparticle(&treemodel);			//Particle system based on this model

  Texture tree(image::load("resource/Tree.png"));
  Texture treenormal(image::load("resource/TreeNormal.png"));

	Buffer modelbuf;
	treeparticle.bind<glm::mat4>("in_Model", &modelbuf);			//Update treeparticle system
  std::vector<glm::mat4> treemodels;

  //Rendering Targets / Framebuffers
  Billboard image(WIDTH, HEIGHT);             //1200x800, color and depth
  Billboard heightimage(WIDTH, HEIGHT);     //1200x800, height levels
  Billboard shadow(8000, 8000);               //800x800, depth only
  Square2D flat;

  //Texture for Hydrology Map Visualization
  Texture map(image::make([&](int i){
    double t1 = 0.0f;
    glm::vec4 color = glm::mix(glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(0.2, 0.5, 1.0, 1.0), t1);
    return color;
  }, ivec2(WSIZE, WSIZE)));

  glm::mat4 mapmodel = glm::mat4(1.0f);


//  mapmodel = glm::translate(mapmodel, glm::vec3(-1.0+0.3*(float)HEIGHT/(float)WIDTH, -1.0+0.3, 0.0));
  mapmodel = glm::scale(mapmodel, glm::vec3(1,1,1)*glm::vec3((float)HEIGHT/(float)WIDTH, 1.0f, 1.0f));
  //model = glm::translate(glm::mat4(1.0), glm::vec3(2.0*pos.x-1.0+scale.x, 2.0*pos.y-1.0+scale.y, 0.0));
  //model = glm::scale(model, glm::vec3(scale.x, scale.y, 1.0));


  //Visualization Hooks
  Tiny::event.handler = [&](){

    cam::handler();

    if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p)
      paused = !paused;

    if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_m)
      viewmap = !viewmap;

    if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_n)
      viewmomentum = !viewmomentum;
  };

  Tiny::view.interface = [](){};

  Tiny::view.pipeline = [&](){

    //Render Shadowmap

    shadow.target();                  //Prepare Target
    depth.use();                      //Prepare Shader
    depth.uniform("dvp", dvp);
    vertexpool.render(GL_TRIANGLES);  //Render Surface Model

    if(!Vegetation::plants.empty()){

      //Render the Trees as a Particle System
      spritedepth.use();
      spritedepth.texture("spriteTexture", tree);
      spritedepth.uniform("om", faceLight);
      spritedepth.uniform("dvp", dvp);
      treeparticle.render();

    }

    heightimage.target(vec3(1,1,1));
    heightshader.use();
    heightshader.uniform("vp", cam::vp);
    vertexpool.render(GL_TRIANGLES);

    //Render Scene to Image

    image.target(skyCol);

    shader.use();
    shader.uniform("vp", cam::vp);
    shader.uniform("dbvp", dbvp);
    shader.texture("shadowMap", shadow.depth);
    shader.uniform("lightCol", lightCol);
    shader.uniform("lightPos", lightPos);
    shader.uniform("lookDir", cam::pos);
    shader.uniform("lightStrength", lightStrength);
    vertexpool.render(GL_TRIANGLES);    //Render Model

    if(!Vegetation::plants.empty()){

      glm::mat4 orient = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f-cam::rot), glm::vec3(0.0, 1.0, 0.0));

      sprite.use();
      sprite.texture("spriteTexture", tree);
      sprite.texture("normalTexture", treenormal);
      sprite.uniform("vp", cam::vp);
      sprite.uniform("om", orient);
      sprite.uniform("faceLight", faceLight);
      sprite.uniform("lightPos", lightPos);
      sprite.uniform("lookDir", cam::pos);

      sprite.uniform("dbvp", dbvp);
      sprite.texture("shadowMap", shadow.depth);
      sprite.uniform("lightCol", lightCol);
      sprite.uniform("lightStrength", lightStrength);


      treeparticle.render();

    }

    //Render to Screen

    Tiny::view.target(skyCol);    //Prepare Target

    effect.use();                             //Prepare Shader
    effect.texture("imageTexture", image.texture);
    effect.texture("depthTexture", image.depth);
    effect.texture("heightTexture", heightimage.texture);
    flat.render();                            //Render Image

    if(viewmap){

      billboard.use();
      billboard.texture("imageTexture", map);
      billboard.uniform("model", mapmodel);
      flat.render();

    }

  };

  int n = 0;
  Tiny::loop([&](){

    if(paused)
      return;

    world.erode(500*FREQUENCY*FREQUENCY); //Execute Erosion Cycles
  //  Vegetation::grow();     //Grow Trees

    for(auto& node: world.map.nodes){
      updatenode<RES>(vertexpool, node);
    }

    cout<<n++<<endl;

    //Update the Tree Particle System
    treemodels.clear();
    for(auto& t: Vegetation::plants){
      glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(t.pos.x, t.size + SCALE*world.map.get(t.pos)->get(t.pos)->height, t.pos.y));
      model = glm::scale(model, glm::vec3(t.size));
      treemodels.push_back(model);
    }
    modelbuf.fill(treemodels);
    treeparticle.SIZE = treemodels.size();    //  cout<<world.trees.size()<<endl;

    //Redraw the Discharge and Momentum Maps
    if(viewmap){

      if(viewmomentum)
      map.raw(image::make([&](int i){
        double t1 = World::map.discharge(vec2(i/WSIZE, i%WSIZE));
        glm::vec4 color = glm::mix(glm::vec4(0.0, 0.0, 0.0, 1.0), glm::vec4(0.2, 0.5, 1.0, 1.0), t1);
        return color;
      }, ivec2(WSIZE, WSIZE)));

      else
      map.raw(image::make([&](int i){

        auto node = world.map.get(math::unflatten(i, ivec2(WSIZE, WSIZE)));
        auto cell = node->get(math::unflatten(i, ivec2(WSIZE, WSIZE)));

        float mx = cell->momentumx;
        float my = cell->momentumy;

        glm::vec4 color = glm::vec4(abs(erf(mx)), 0, abs(erf(my)), 1.0);

        return color;
      }, ivec2(WSIZE, WSIZE)));
    }

  });

  return 0;
}
