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
