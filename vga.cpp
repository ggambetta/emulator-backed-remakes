#include "vga.h"
#include "x86.h"

#include <iostream>
#include <fstream>

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


static const byte kCGAColors[2][4][3] = {
  { // Palette 0
    { 0x00, 0x00, 0x00 },  // Black
    { 0x00, 0xAA, 0x00 },  // Green
    { 0xAA, 0x00, 0x00 },  // Red
    { 0xAA, 0x55, 0x00 },  // Brown
  },
  { // Palette 1
    { 0x00, 0x00, 0x00 },  // Black
    { 0x00, 0xAA, 0xAA },  // Cyan
    { 0xAA, 0x00, 0xAA },  // Magenta
    { 0xAA, 0xAA, 0xAA },  // Gray
  },
};



VGA::VGA(X86* x86) : x86_(x86), mode_(0) {
  x86_->registerInterruptHandler(this, 0x10);
}

 
void VGA::handleInterrupt(int num) {
  ASSERT(num == 0x10);
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
    clearVRAM();
  } else {
    cerr << "VGA: Unsupported video mode 0x" << Hex8 << mode << endl;
  }
}


void VGA::clearVRAM() {
  if (mode_ == MODE_CGA_320x200) {
    byte* vram = x86_->getMem8Ptr(0xB800, 0);
    memset(vram, 0, 16384);
  } else {
    cerr << "VGA: Unsupported video mode 0x" << Hex8 << mode_ << endl;
  }
}


void VGA::randomVRAM() {
  if (mode_ == MODE_CGA_320x200) {
    byte* vram = x86_->getMem8Ptr(0xB800, 0);
    for (int i = 0; i < 16384; i++) {
      *vram++ = rand() & 0xFF;
    }
  } else {
    cerr << "VGA: Unsupported video mode 0x" << Hex8 << mode_ << endl;
  }
}


void VGA::setPalette(int palette) {
  if (mode_ == MODE_CGA_320x200) {
    clog << "Setting CGA palette " << palette << endl;
    cga_palette_ = palette;
  }
}


void VGA::renderRGB(byte* buffer) {
  if (mode_ != MODE_CGA_320x200) {
    clog << "Can't render screen in mode " << (int)mode_ << endl;
    return;
  }

  byte* vram = x86_->getMem8Ptr(0xB800, 0);
  for (int y = 0; y < 200; y++) {
    // CGA video memory is interlaced: all even rows, then all odd rows
    // with a 8K offset.
    byte* vram_row = vram + (y/2)*320/4 + (y%2)*8192;

    for (int x = 0; x < 320; x += 4) {
      byte val = *vram_row++; 

      // 4 pixels per byte.
      byte mask = 0b11000000;
      int shift = 6;
      for (int px = 0; px < 4; px++) {
        byte color = (val & mask) >> shift;
        shift -= 2;
        mask >>= 2;
  
        *buffer++ = kCGAColors[cga_palette_][color][0];
        *buffer++ = kCGAColors[cga_palette_][color][1];
        *buffer++ = kCGAColors[cga_palette_][color][2];
      }
    }
  }
}


void VGA::getModeSize(int& width, int& height) {
  width = height = 0;
  if (mode_ == MODE_CGA_320x200) {
    width = 320;
    height = 200;
  }
}


float VGA::getPixelAspectRatio() {
  if (mode_ == MODE_CGA_320x200) {
    return 1.2f;
  }
  return 1.0f;
}
