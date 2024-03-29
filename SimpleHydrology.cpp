#include <TinyEngine/TinyEngine>
#include <TinyEngine/camera>
#include <TinyEngine/image>

#include "source/vertexpool.h"
#include "source/world.h"
#include "source/model.h"

#include <random>

mappool::pool<quad::cell> cellpool;
Vertexpool<Vertex> vertexpool;

int main( int argc, char* args[] ) {

  assert(TINYENGINE_VERSION == "1.7");

  Tiny::view.vsync = false;
  Tiny::view.blend = false;
  Tiny::window("Simple Hydrology", WIDTH, HEIGHT);
  glDisable(GL_CULL_FACE);

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

  // Initialize the Visualization

  // Camera

  cam::near = -800.0f;
  cam::far = 800.0f;
  cam::moverate = 10.0f;
  cam::look = glm::vec3(quad::size/2, quad::mapscale/2, quad::size/2);
  cam::roty = 60.0f;
  cam::rot = 180.0f;
  cam::init(3, cam::ORTHO);
  cam::update();

  //Setup Shaders

  Shader defaultshader({"source/shader/default.vs", "source/shader/default.fs"}, {"in_Position", "in_Normal", "in_Tangent", "in_Bitangent"});
  Shader defaultdepth({"source/shader/depth.vs", "source/shader/depth.fs"}, {"in_Position"});

  Shader treeshader({"source/shader/tree.vs", "source/shader/tree.fs"}, {"in_Pos", "in_Model"});
  Shader treedepth({"source/shader/treedepth.vs", "source/shader/treedepth.fs"}, {"in_Pos", "in_Model"});

  Shader ssaoshader({"source/shader/ssao.vs", "source/shader/ssao.fs"}, {"in_Quad", "in_Tex"});
  Shader imageshader({"source/shader/image.vs", "source/shader/image.fs"}, {"in_Quad", "in_Tex"});
  Shader mapshader({"source/shader/map.vs", "source/shader/map.fs"}, {"in_Quad", "in_Tex"});

  //Rendering Targets / Framebuffers

  Billboard image(WIDTH, HEIGHT);             //1200x800, color and depth

  Texture shadowmap(8000, 8000, {GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT});
  Target shadow(8000, 8000);
  shadow.bind(shadowmap, GL_DEPTH_ATTACHMENT);

  Square2D flat;

  // SSAO

  Texture gPosition(WIDTH, HEIGHT, {GL_RGBA16F, GL_RGBA, GL_FLOAT});
  Texture gNormal(WIDTH, HEIGHT, {GL_RGBA16F, GL_RGBA, GL_FLOAT});
  Texture gColor(WIDTH, HEIGHT, {GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE});
  Texture gDepth(WIDTH, HEIGHT, {GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE});

  Target gBuffer(WIDTH, HEIGHT);
  gBuffer.bind(gPosition, GL_COLOR_ATTACHMENT0);
  gBuffer.bind(gNormal, GL_COLOR_ATTACHMENT1);
  gBuffer.bind(gColor, GL_COLOR_ATTACHMENT2);
  gBuffer.bind(gDepth, GL_DEPTH_ATTACHMENT);

  Texture ssaotex(WIDTH, HEIGHT, {GL_RED, GL_RED, GL_FLOAT});
  Target ssaofbo(WIDTH, HEIGHT);
  ssaofbo.bind(ssaotex, GL_COLOR_ATTACHMENT0);

  // generate sample kernel
  // ----------------------
  std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
  std::default_random_engine generator;
  std::vector<glm::vec3> ssaoKernel;
  for (unsigned int i = 0; i < 64; ++i){
      glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
      sample = glm::normalize(sample);
      sample *= randomFloats(generator);
      float scale = float(i) / 64.0f;
      scale = 0.1f + scale*scale*(1.0f-0.1f);
      sample *= scale;
      ssaoKernel.push_back(sample);
  }

  // generate noise texture
  // ----------------------
  std::vector<glm::vec3> ssaoNoise;
  for (unsigned int i = 0; i < 16; i++) {
      glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
      ssaoNoise.push_back(noise);
  }

  Texture noisetex(4, 4, {GL_RGBA32F, GL_RGB, GL_FLOAT}, &ssaoNoise[0]);
  glBindTexture(GL_TEXTURE_2D, noisetex.texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);



  // Lets try an alternative tree model:
  //  a cone! Is visible from the top.

  Model conemodel({"in_Pos", "in_Normal"});
  std::vector<glm::vec4> conepos;
  std::vector<glm::vec3> conenormal;

  for(int i = 0; i < 16; i++){

    float phiA = 2.0f*3.14159265f*(float)i/15.0f;
    float phiB = 2.0f*3.14159265f*(float)(i+1)/15.0f;

    conepos.push_back(vec4(sin(phiA), -1, cos(phiA), 1));
    conepos.push_back(vec4(sin(phiB), -1, cos(phiB), 1));
    conepos.push_back(vec4(0, 1, 0, 1));

    conenormal.push_back(vec3(sin(phiA), 0.25, cos(phiA)));
    conenormal.push_back(vec3(sin(phiB), 0.25, cos(phiB)));
    conenormal.push_back(vec3(sin(0.5f*(phiA + phiB)), 0.25, cos(0.5f*(phiA + phiB))));

  }

  Buffer coneposbuf(conepos);
  Buffer conenormalbuf(conenormal);
  conemodel.bind<vec4>("in_Pos", &coneposbuf);
  conemodel.bind<vec3>("in_Normal", &conenormalbuf);
  conemodel.SIZE = 16*3;

  //Trees as a Particle System

  Instance treeparticle(&conemodel);	//Particle system based on this model
  Buffer modelbuf;
  treeparticle.bind<glm::mat4>("in_Model", &modelbuf);			//Update treeparticle system
  std::vector<glm::mat4> treemodels;

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

  Tiny::view.interface = [](){
    ImGui::SetNextWindowSize(ImVec2(480, 260), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(50, 470), ImGuiCond_Once);
    ImGui::Begin("SimpleHydrology", NULL, ImGuiWindowFlags_NoResize);
    ImGui::ColorEdit3("Flat Color", &flatColor[0]);
    ImGui::ColorEdit3("Steep Color", &steepColor[0]);
    ImGui::ColorEdit3("Water Color", &waterColor[0]);
    ImGui::ColorEdit3("Sky Color", &skyCol[0]);
    ImGui::ColorEdit3("Tree Color", &treeColor[0]);
    ImGui::DragFloat("lightStrength", &lightStrength);
    ImGui::DragFloat("ssaoradius", &ssaoradius);
    if(ImGui::DragFloat3("lightPos", &lightPos[0])){

      dv = glm::lookAt(worldcenter + normalize(vec3(lightPos.x, lightPos.y, lightPos.z)), worldcenter, glm::vec3(0,1,0));
      bias = glm::mat4(
          0.5, 0.0, 0.0, 0.0,
          0.0, 0.5, 0.0, 0.0,
          0.0, 0.0, 0.5, 0.0,
          0.5, 0.5, 0.5, 1.0
    );
      dvp = dp*dv;
      dbvp = bias*dvp;

    }
    ImGui::End();
  };

  Tiny::view.pipeline = [&](){

    // Render gBuffer geometry pass

    gBuffer.target(vec3(0));
    defaultshader.use();
    defaultshader.uniform("proj", cam::proj);
    defaultshader.uniform("view", cam::view);
    defaultshader.texture("dischargeMap", dischargeMap);
    defaultshader.texture("normalMap", normalMap);
    defaultshader.uniform("flatColor", flatColor);
    defaultshader.uniform("waterColor", waterColor);
    defaultshader.uniform("steepColor", steepColor);
    vertexpool.render(GL_TRIANGLES);

    if(!Vegetation::plants.empty()){

      glm::mat4 orient = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f-cam::rot), glm::vec3(0.0, 1.0, 0.0));

      treeshader.use();
      treeshader.uniform("proj", cam::proj);
      treeshader.uniform("view", cam::view);
      treeshader.uniform("color", treeColor);
      treeparticle.render(GL_TRIANGLES);

    }

    // SSAO Texture

    ssaofbo.target(vec3(0));
    ssaoshader.use();
    for (unsigned int i = 0; i < 64; ++i)
      ssaoshader.uniform("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    ssaoshader.uniform("projection", cam::proj);
    ssaoshader.texture("gPosition", gPosition);
    ssaoshader.texture("gNormal", gNormal);
    ssaoshader.texture("texNoise", noisetex);
    ssaoshader.uniform("radius", ssaoradius);
    flat.render();

    //Render Shadowmap

    shadow.target();                  //Prepare Target
    defaultdepth.use();                      //Prepare Shader
    defaultdepth.uniform("dvp", dvp);
    vertexpool.render(GL_TRIANGLES);  //Render Surface Model

    if(!Vegetation::plants.empty()){

      //Render the Trees as a Particle System
      treedepth.use();
      treedepth.uniform("dvp", dvp);
      treeparticle.render(GL_TRIANGLES);

    }

    //Render Scene to Screen

    Tiny::view.target(skyCol);    //Prepare Target
    imageshader.use();
    imageshader.texture("gPosition", gPosition);
    imageshader.texture("gNormal", gNormal);
    imageshader.texture("gColor", gColor);
    imageshader.texture("gDepth", gDepth);
    imageshader.texture("ssaoTex", ssaotex);
    imageshader.texture("shadowMap", shadowmap);
    imageshader.texture("dischargeMap", dischargeMap);
    imageshader.uniform("view", cam::view);
    imageshader.uniform("dbvp", dbvp);
    imageshader.uniform("lightCol", lightCol);
    imageshader.uniform("skyCol", skyCol);
    imageshader.uniform("lightPos", lightPos);
    imageshader.uniform("lookDir", cam::pos);
    imageshader.uniform("lightStrength", lightStrength);
    flat.render();

    //Render Map to Screen

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
  //    if(World::map.height(p) < 0.3)
  //      d = 1.0;
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
