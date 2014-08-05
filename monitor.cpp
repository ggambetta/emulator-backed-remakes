#include "monitor.h"
#include "vga.h"

#include <SDL2/SDL.h>
#include <fstream>

using namespace std;

Monitor::Monitor(VGA* vga)
  : vga_(vga), scale_(1), window_(nullptr), renderer_(nullptr) {

}


Monitor::~Monitor() {
  closeWindow();
}


void Monitor::setScale(int scale) {
  if (scale >= 1) {
    scale_ = scale;
  }
}


void Monitor::saveToFile(const std::string& filename) {
  int width, height;
  vga_->getModeSize(width, height);
  if (width == 0 || height == 0) {
    return;
  }

  byte* buffer = new byte[width*height*3];
  vga_->renderRGB(buffer);

  ofstream file(filename);
  file << "P6\n";
  file << "320 200\n";
  file << "255\n";
  file.write((const char*)buffer, width*height*3);
  file.close();

  delete[] buffer;
}


void Monitor::closeWindow() {
  if (!window_) {
    return;
  }
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  renderer_ = nullptr;
  window_ = nullptr;
}


void Monitor::update() {
  int width, height;
  vga_->getModeSize(width, height);

  // Unsupported video mode.
  if (width == 0 || height == 0) {
    closeWindow();
    return;
  }

  // If the window size changed, destroy the old window.
  int req_width = scale_*width;
  int req_height = scale_*height*vga_->getPixelAspectRatio();

  if (window_) {
    int current_width, current_height;
    SDL_GetWindowSize(window_, &current_width, &current_height);
    if (current_width != req_width || current_height != req_height) {
      closeWindow();
    }
  }

  // Create the window if necessary.
  if (!window_) {
    SDL_CreateWindowAndRenderer(req_width, req_height, 0, &window_, &renderer_);
  }

  // Render to a buffer.
  byte* buffer = new byte[width*height*3];
  vga_->renderRGB(buffer);

  // Wrap in a texture.
  SDL_Surface* surface = 
      SDL_CreateRGBSurfaceFrom(buffer, width, height, 24, width*3,
                               0x0000FF, 0x00FF00, 0xFF0000, 0);

  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);

  // Render.
  SDL_RenderCopy(renderer_, texture, NULL, NULL);
  SDL_RenderPresent(renderer_);

  // Release memory.
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
  delete [] buffer;
}
