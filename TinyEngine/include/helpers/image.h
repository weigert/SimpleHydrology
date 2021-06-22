namespace image {

  SDL_Surface* load(std::string path){
    SDL_Surface* loaded = IMG_Load(path.c_str());
    if(loaded == NULL)
      printf( "Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
    SDL_Surface* optimized = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(loaded);
    return optimized;
  }

  void save(SDL_Surface* surface, std::string path){
    IMG_SavePNG(surface, path.c_str());
  }

  template<typename T>
  SDL_Surface* make(glm::ivec2 size, T* data, std::function<glm::vec4(T)> handle){
    SDL_Surface *s = SDL_CreateRGBSurface(0, size.x, size.y, 32, 0, 0, 0, 0);
    SDL_LockSurface(s);

    unsigned char* img_raw = (unsigned char*)s->pixels; //Raw Data

    for(int i = 0; i < size.x*size.y; i++){
      glm::vec4 color = handle(data[i]);  //Construct from Algorithm
      *(img_raw+4*i)    = (unsigned char)(255*color.x);
      *(img_raw+4*i+1)  = (unsigned char)(255*color.y);
      *(img_raw+4*i+2)  = (unsigned char)(255*color.z);
      *(img_raw+4*i+3)  = (unsigned char)(255*color.w);
    }

    SDL_UnlockSurface(s);
    return s;
  }

  template<typename T>
  SDL_Surface* make(glm::ivec2 size, T* data1, T* data2, std::function<glm::vec4(T, T)> handle){
    SDL_Surface *s = SDL_CreateRGBSurface(0, size.x, size.y, 32, 0, 0, 0, 0);
    SDL_LockSurface(s);

    unsigned char* img_raw = (unsigned char*)s->pixels; //Raw Data

    for(int i = 0; i < size.x*size.y; i++){
      glm::vec4 color = handle(data1[i], data2[i]);  //Construct from Algorithm
      *(img_raw+4*i)    = (unsigned char)(255*color.x);
      *(img_raw+4*i+1)  = (unsigned char)(255*color.y);
      *(img_raw+4*i+2)  = (unsigned char)(255*color.z);
      *(img_raw+4*i+3)  = (unsigned char)(255*color.w);
    }

    SDL_UnlockSurface(s);
    return s;
  }


  template<typename T>
  SDL_Surface* makeT(glm::ivec2 size, T* data1, T* data2, std::function<glm::vec4(T, T)> handle){
    SDL_Surface *s = SDL_CreateRGBSurface(0, size.x, size.y, 32, 0, 0, 0, 0);
    SDL_LockSurface(s);

    unsigned char* img_raw = (unsigned char*)s->pixels; //Raw Data

    // transpose!
    for (int x=0, i=0; x<size.x; x++)
    for (int y=0; y<size.y; y++, i++)
    {
      int j = x+y*size.x;
      glm::vec4 color = handle(data1[i], data2[i]);  //Construct from Algorithm
      *(img_raw+4*j)    = (unsigned char)(255*color.x);
      *(img_raw+4*j+1)  = (unsigned char)(255*color.y);
      *(img_raw+4*j+2)  = (unsigned char)(255*color.z);
      *(img_raw+4*j+3)  = (unsigned char)(255*color.w);
    }

    SDL_UnlockSurface(s);
    return s;
  }
};
