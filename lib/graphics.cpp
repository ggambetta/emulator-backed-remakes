#include "graphics.h"

#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>

using namespace std;

//
// An Image.
//
Image::Image(const string& filename) : texture_(nullptr) {
  surface_ = IMG_Load(filename.data());
  width_ = surface_->w;
  height_ = surface_->h;
}


Image::~Image() {
  if (texture_) {
    SDL_DestroyTexture(texture_);
  }
  if (surface_) {
    SDL_FreeSurface(surface_);
  }
}


int Image::getWidth() const {
  return width_;
}


int Image::getHeight() const {
  return height_;
}


SDL_Texture* Image::getTexture(SDL_Renderer* renderer) {
  if (!texture_) {
    texture_ = SDL_CreateTextureFromSurface(renderer, surface_);
    SDL_FreeSurface(surface_);
    surface_ = nullptr;
  }
  return texture_;
}


// 
// A Window.
//
Window::Window(int width, int height, const string& title) {
  SDL_CreateWindowAndRenderer(width, height, 0, &window_, &renderer_);
  SDL_SetWindowTitle(window_, title.data());
  buffer_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
                              SDL_TEXTUREACCESS_TARGET, width, height);

  SDL_SetRenderTarget(renderer_, buffer_);
}


Window::~Window() {
  SDL_DestroyTexture(buffer_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);

  renderer_ = nullptr;
  window_ = nullptr;
}


void Window::update() {

  SDL_SetRenderTarget(renderer_, nullptr);
  SDL_RenderCopy(renderer_, buffer_, nullptr, nullptr);

  SDL_RenderPresent(renderer_);
  SDL_SetRenderTarget(renderer_, buffer_);

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    // TODO: consume input events and feed them back to the emulator.
  }
}


void Window::drawImage(Image* image, int x, int y) {
  SDL_Texture* texture = image->getTexture(renderer_);

  SDL_Rect dst;
  dst.x = x;
  dst.y = y;
  dst.w = image->getWidth();
  dst.h = image->getHeight();

  SDL_RenderCopy(renderer_, texture, NULL, &dst); 
}

