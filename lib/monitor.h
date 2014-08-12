// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <string>

class VGA;
struct SDL_Window;
struct SDL_Renderer;

class Monitor {
 public:
  Monitor(VGA* vga);
  virtual ~Monitor();

  void update();
  void closeWindow();

  void setScale(int scale);

  void savePPM(const std::string& filename);

 private:
  VGA* vga_;
  int scale_;

  SDL_Window* window_;
  SDL_Renderer* renderer_;
};

#endif  // __MONITOR_H__
