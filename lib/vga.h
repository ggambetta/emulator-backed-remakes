// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
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

  void setVideoMode(int mode);
  void setPalette(int palette);

  void clearVRAM();
  void randomVRAM();

  static void CGAtoRGB(byte* cga, int palette, int npixels, byte* rgb);

 private:
  X86* x86_;

  byte mode_;
  byte cga_palette_;
};

#endif // __VGA_H__
