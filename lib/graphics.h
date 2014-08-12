// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__ 

#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;

//
// An Image.
//
class Image {
 public:
  Image(const std::string& filename);
  ~Image();

  int getWidth() const;
  int getHeight() const;

  SDL_Texture* getTexture(SDL_Renderer* renderer);

 private:
  SDL_Surface* surface_;
  SDL_Texture* texture_;
  int width_;
  int height_;
};


// 
// A Window.
//
class Window {
 public:
  Window(int width, int height, const std::string& title);
  ~Window();

  void drawImage(Image* image, int x, int y);

  void update();

 private:
  SDL_Window* window_;
  SDL_Renderer* renderer_;
  SDL_Texture* buffer_;
};

#endif // __GRAPHICS_H__
