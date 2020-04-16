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

  //Setup Rendering Billboards
  Billboard shadow(800, 800, true); //800x800, depth only
  Billboard image(WIDTH, HEIGHT, false); //1200x800, depth only

  //Setup 2D Images
  Billboard path(256, 256, false); //Render target for automata
  Billboard pool(256, 256, false); //Render target for automata
  path.raw(image::make<double>(glm::vec2(256, 256), world.waterpath, pathColor));
  pool.raw(image::make<double>(glm::vec2(256, 256), world.waterpool, poolColor));

  //Setup World Model
  Model model(constructor);
  model.translate(glm::vec3(-128.0, 0.0, -128.0));

  //Visualization Hooks
  Tiny::event.handler = eventHandler;
	Tiny::view.interface = [](){};
  Tiny::view.pipeline = [&](){

    //Render Shadowmap
    shadow.target();                  //Prepare Target
    depth.use();                      //Prepare Shader
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
    billboard.use();
    glActiveTexture(GL_TEXTURE0+0);
    glBindTexture(GL_TEXTURE_2D, image.depthTexture);
    billboard.setInt("imageTexture", 0);
    image.move(glm::vec2(0.0, 0.8), glm::vec2(0.2));
    billboard.setMat4("model", image.model);
    image.render();

    //Path Visualization
    glBindTexture(GL_TEXTURE_2D, path.texture);
    path.move(glm::vec2(0.0, 0.0), glm::vec2(0.4));
    billboard.setMat4("model", path.model);
    path.render();

    //Pool Visualization
    glBindTexture(GL_TEXTURE_2D, pool.texture);
    pool.move(glm::vec2(0.6, 0.0), glm::vec2(0.4));
    billboard.setMat4("model", pool.model);
    pool.render();
  };

  //Define a World Mesher?

  Tiny::loop([&](){
    //Do Erosion Cycles!
    if(!paused){
      //Erode the World and Update the Model
      world.erode(250);
      model.construct(constructor); //Reconstruct Updated Model

      //Redraw the Path and Death Image
      path.raw(image::make<double>(glm::vec2(256, 256), world.waterpath, pathColor));
      pool.raw(image::make<double>(glm::vec2(256, 256), world.waterpool, poolColor));
    }
  });

  return 0;
}

/*

Appropriate System Scale:

Map is given by X*Y (256x256)

We spawn 100 particles per round.

Every round, there is a uniform probability of spawning at any location.
P = 1/(XY)

What does 0 mean? -> Water absolutely never gets there
What does 1 mean? -> Water always crosses there?

But after how long do we consider the water evaporated?

And how much does it contribute?

PATH = PATHOLD*DECAY + (1-DECAY)*PARTICLE?
for all path. This is expensive though lol to do for every guy all the time.



*/
