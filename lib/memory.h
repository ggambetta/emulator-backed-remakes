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
