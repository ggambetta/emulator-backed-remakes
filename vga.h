#ifndef __VGA_H__
#define __VGA_H__

#include "device.h"

class X86;

class VGA : public InterruptHandler, public IOHandler {
 public:
  VGA(X86* x86);
 
  virtual void handleInterrupt(int num);
  virtual byte handleIN(int port);
  virtual void handleOUT(int port, byte val);

  virtual void getModeSize(int& width, int& height);
  virtual float getPixelAspectRatio();
  virtual void renderRGB(byte* buffer);

 private:
  void setVideoMode(int mode);
  void setPalette(int palette);

 private:
  X86* x86_;

  byte mode_;
  byte cga_palette_;
};

#endif // __VGA_H__
