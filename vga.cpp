#include "vga.h"
#include "x86.h"

#include <cassert>
#include <iostream>

using namespace std;

enum {
  MODE_BW40 = 0,      // Text 40x25
  MODE_CO40,          // Text 40x25
  MODE_BW80,          // Text 80x25
  MODE_C080,          // Text 80x25

  MODE_CGA_320x200,   // CGA 320x200 Color
  MODE_CGA_320x200_G, // CGA 320x200 Greyscale

  MODE_CGA_640x200
};


VGA::VGA(X86* x86) : x86_(x86), mode_(0) {
  x86_->registerInterruptHandler(this, 0x10);
}

 
void VGA::handleInterrupt(int num) {
  assert(num == 0x10);
  Registers* regs = x86_->getRegisters();

  if (regs->ah == 0x00) {
    setVideoMode(regs->al);
  } else if (regs->ah == 0x0B) {
    setPalette(regs->al);
  } else {
    cerr << "VGA: Unknown interrupt command 0x" << Hex8 << (int)regs->ah << endl;
  }
}


byte VGA::handleIN(int port) {
  cerr << "VGA: Unhandled IN 0x" << Hex16 << (int)port << endl;
  return 0;
}


void VGA::handleOUT(int port, byte val) {
  cerr << "VGA: Unhandled OUT 0x" << Hex16 << (int)port << endl;
}


void VGA::setVideoMode(int mode) {
  mode_ = mode;
  
  if (mode_ == MODE_CGA_320x200) {
    clog << "VGA: Video mode 0x" << Hex8 << mode_ << " (CGA 320x200)" << endl;
    cga_palette_ = 0;
  } else {
    cerr << "VGA: Unsupported video mode 0x" << Hex8 << mode << endl;
  }
}


void VGA::setPalette(int palette) {
  if (mode_ == MODE_CGA_320x200) {
    clog << "Setting CGA palette " << palette << endl;
  }
}
 

