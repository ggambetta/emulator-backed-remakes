// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#include "memory.h"

#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;

Memory::Memory(int size) {
  size_ = size;
  data_ = new byte[size];
  memset(data_, 0, size_);
}

Memory::~Memory() {
  delete [] data_;
  data_ = nullptr;
}

byte Memory::read(int address) const {
  return data_[address];
}

void Memory::write(int address, byte value) {
  if (address >= size_) {
    stringstream ss;
    ss << "Attempt to write at " << address << ", size is " << size_;
    FATAL(ss.str());
  }
  data_[address] = value;
}

byte* Memory::getPointer(int address) {
  if (address >= size_) {
    stringstream ss;
    ss << "Attempt to get a pointer to " << address << ", size is " << size_;
    FATAL(ss.str());
  }
  return data_ + address;
}

int Memory::getSize() const {
  return size_;
}
