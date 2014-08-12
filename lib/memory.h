// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "helpers.h"

//
// Memory.
//
class Memory {
 public:
  Memory(int size);
  ~Memory();

  byte read(int address) const;
  void write(int address, byte value);

  byte* getPointer(int address);
  int getSize() const;

 private:
  byte* data_;
  int size_;
};

#endif  // __MEMORY_H__
