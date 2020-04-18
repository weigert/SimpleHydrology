#include "TinyEngine/TinyEngine.h"
#include <noise/noise.h>
#include "source/world.h" //Model

int main( int argc, char* args[] ) {

  //Generate the World
  world.generate();

  //Initialize the Visualization
  Tiny::init("River Systems Simulator", WIDTH, HEIGHT);

  //Setup Shaders
  Shader shader("source/shader/default.vs", "source/shader/default.fs", {"in_Position", "in_Normal", "in_Color"});
  Shader depth("source/shader/depth.vs", "source/shader/depth.fs", {"in_Position"});
  Shader effect("source/shader/effect.vs", "source/shader/effect.fs", {"in_Quad", "in_Tex"});
  Shader billboard("source/shader/billboard.vs", "source/shader/billboard.fs", {"in_Quad", "in_Tex"});
  Shader sprite("source/shader/sprite.vs", "source/shader/sprite.fs", {"in_Quad", "in_Tex"});

  //Sprite
  Sprite tree(image::load("Tree.png"));

  //Setup Rendering Billboards
  Billboard shadow(1200, 1200, true); //800x800, depth only
  Billboard image(WIDTH, HEIGHT, false); //1200x800, depth only

  //Setup 2D Images
  Billboard map(world.dim.x, world.dim.y, false); //Render target for automata
  map.raw(image::make<double>(world.dim, world.waterpath, world.waterpool, hydromap));

  //Setup World Model
  Model model(constructor);
  model.translate(-viewPos);

  //Visualization Hooks
  Tiny::event.handler = eventHandler;
	Tiny::view.interface = [](){};
  Tiny::view.pipeline = [&](){

    //Render Shadowmap
    shadow.target();                  //Prepare Target
    depth.use();                      //Prepare Shader
    model.model = glm::translate(glm::mat4(1.0), -viewPos);
    depth.setMat4("dmvp", depthProjection * depthCamera * model.model);
    model.render(GL_TRIANGLES);       //Render Model

    //Regular Image
    image.target(skyCol);           //Prepare Target
    shader.use();                   //Prepare Shader
    glActiveTexture(GL_TEXTURE0+0);
    glBindTexture(GL_TEXTURE_2D, shadow.depthTexture);
    shader.setInt("shadowMap", 0);
    shader.setVec3("lightCol", lightCol);
    shader.setVec3("lightPos", lightPos);
    shader.setVec3("lookDir", lookPos-cameraPos);
    shader.setFloat("lightStrength", lightStrength);
    shader.setMat4("projectionCamera", projection * camera);
    shader.setMat4("dbmvp", biasMatrix * depthProjection * depthCamera * glm::mat4(1.0f));
    shader.setMat4("model", model.model);
    shader.setVec3("flatColor", flatColor);
    shader.setVec3("steepColor", steepColor);
    shader.setFloat("steepness", steepness);
    model.render(GL_TRIANGLES);    //Render Model

    //Render the Trees
    if(!world.trees.empty()){

      sprite.use();
      glActiveTexture(GL_TEXTURE0+0);
      glBindTexture(GL_TEXTURE_2D, tree.texture);
      sprite.setInt("spriteTexture", 0);
      sprite.setMat4("projectionCamera", projection*camera);
      sprite.setVec3("lightPos", lightPos);
      glm::mat4 M = glm::rotate(glm::mat4(1.0), glm::radians(rotation - 45.0f), glm::vec3(0.0, 1.0, 0.0));
      sprite.setVec3("lookDir", M*glm::vec4(cameraPos, 1.0));

      //Render all trees in the scene
      for(auto& t: world.trees){
        glm::vec3 shift = glm::vec3(0.0, t.size, 0.0);  //Starts at Base
        shift += glm::vec3(0.0, world.scale*world.heightmap[t.index], 0.0);

        //Move the Model
        tree.model = glm::translate(glm::mat4(1.0), glm::vec3(t.pos.x, 0.0, t.pos.y) - viewPos + shift);
        tree.model = glm::rotate(tree.model, -glm::radians(rotation - 45.0f), glm::vec3(0.0, 1.0, 0.0));
        tree.model = glm::scale(tree.model, glm::vec3(t.size));
        sprite.setMat4("model", tree.model);
        tree.render();
      }
    }

    //Render to Screen
    Tiny::view.target(color::black);    //Prepare Target
    effect.use();                //Prepare Shader
    glActiveTexture(GL_TEXTURE0+0);
    glBindTexture(GL_TEXTURE_2D, image.texture);
    effect.setInt("imageTexture", 0);
    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D, image.depthTexture);
    effect.setInt("depthTexture", 1);
    image.render();                     //Render Image

    //Render Additional Information
    if(viewmap){
      billboard.use();
      glActiveTexture(GL_TEXTURE0+0);
      glBindTexture(GL_TEXTURE_2D, image.depthTexture);
      billboard.setInt("imageTexture", 0);
      image.move(glm::vec2(0.0, 0.8), glm::vec2(0.2));
      billboard.setMat4("model", image.model);
      image.render();

      glBindTexture(GL_TEXTURE_2D, map.texture);
      map.move(glm::vec2(0.8, 0.8), glm::vec2(0.2));
      billboard.setMat4("model", map.model);
      map.render();
    }
  };

  //Define a World Mesher?

  Tiny::loop([&](){
    //Do Erosion Cycles!
    if(!paused){
      //Erode the World and Update the Model
      world.erode(250);
      world.grow();
      model.construct(constructor); //Reconstruct Updated Model

      //Redraw the Path and Death Image
      if(viewmap)
        map.raw(image::make<double>(world.dim, world.waterpath, world.waterpool, hydromap));
    }
  });

  return 0;
}
