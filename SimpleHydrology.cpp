#include <TinyEngine/TinyEngine>
#include <TinyEngine/camera>
#include <TinyEngine/image>

#include "source/model.h"
#include "source/vertexpool.h"
#include "source/world.h"

mappool::pool<quad::cell> cellpool;
Vertexpool<Vertex> vertexpool;

int main( int argc, char* args[] ) {

  Tiny::view.vsync = false;
  Tiny::view.antialias = 16;
  Tiny::window("Simple Hydrology", WIDTH, HEIGHT);

  //Initialize the World

  World world;

  if(argc >= 2){
    World::SEED = std::stoi(args[1]);
    srand(std::stoi(args[1]));
  }
  else {
    World::SEED = time(NULL);
    srand(World::SEED);
  }



  cellpool.reserve(quad::area);
  vertexpool.reserve(quad::tilearea, quad::maparea);

  World::map.init(vertexpool, cellpool, World::SEED);

  //Vertexpool for Drawing Surface

  for(auto& node: world.map.nodes){
    updatenode(vertexpool, node);
  }











  //Initialize the Visualization


  glDisable(GL_CULL_FACE);

  cam::near = -800.0f;
  cam::far = 800.0f;
  cam::moverate = 10.0f;
  cam::look = glm::vec3(quad::size/2, quad::mapscale/2, quad::size/2);
  cam::roty = 45.0f;
  cam::rot = 180.0f;
  cam::init(3, cam::ORTHO);
  cam::update();

  //Setup Shaders
  Shader shader({"source/shader/default.vs", "source/shader/default.fs"}, {"in_Position", "in_Normal", "in_Color"});
  Shader depth({"source/shader/depth.vs", "source/shader/depth.fs"}, {"in_Position"});
  Shader effect({"source/shader/effect.vs", "source/shader/effect.fs"}, {"in_Quad", "in_Tex"});
  Shader mapshader({"source/shader/map.vs", "source/shader/map.fs"}, {"in_Quad", "in_Tex"});
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

  //Texture for Hydrological Map Visualization

  Texture normalMap(image::load("resource/normal.png"));

  Texture momentumMap(image::make([&](const ivec2 p){
    return vec4(0,0,0,0);
  }, quad::res));

  Texture dischargeMap(image::make([&](const ivec2 p){
    return vec4(0,0,0,0);
  }, quad::res));

  glm::mat4 mapmodel = glm::mat4(1.0f);
  mapmodel = glm::scale(mapmodel, glm::vec3(1,1,1)*glm::vec3((float)HEIGHT/(float)WIDTH, 1.0f, 1.0f));

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
    shader.texture("dischargeMap", dischargeMap);
    shader.texture("normalMap", normalMap);

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

      mapshader.use();
      mapshader.texture("momentumMap", momentumMap);
      mapshader.texture("dischargeMap", dischargeMap);
      mapshader.uniform("model", mapmodel);
      mapshader.uniform("view", viewmomentum);
      flat.render();

    }

  };

  int n = 0;
  Tiny::loop([&](){

    if(paused)
      return;

    world.erode(quad::tilesize); //Execute Erosion Cycles
    Vegetation::grow();     //Grow Trees

    for(auto& node: world.map.nodes){
      updatenode(vertexpool, node);
    }

    cout<<n++<<endl;

    //Update the Tree Particle System

    treemodels.clear();
    for(auto& t: Vegetation::plants){
      glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(t.pos.x, t.size + quad::mapscale*world.map.get(t.pos)->get(t.pos)->height, t.pos.y));
      model = glm::scale(model, glm::vec3(t.size));
      treemodels.push_back(model);
    }
    modelbuf.fill(treemodels);
    treeparticle.SIZE = treemodels.size();    //  cout<<world.trees.size()<<endl;

    // Update Maps

    dischargeMap.raw(image::make([&](const ivec2 p){
      double d = World::map.discharge(p);
      if(World::map.height(p) < 0.1)
        d = 1.0;
      return vec4(waterColor, d);
    }, quad::res));

    momentumMap.raw(image::make([&](const ivec2 p){
      auto node = world.map.get(p);
      auto cell = node->get(p);
      float mx = cell->momentumx;
      float my = cell->momentumy;
      return glm::vec4(0.5f*(1.0f+erf(mx)), 0.5f*(1.0f+erf(my)), 0.5f, 1.0);
    }, quad::res));

  });

  return 0;
}
