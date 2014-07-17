#include "memory.h"

#include <cstring>
#include <cassert>
#include <iostream>

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
    cerr << "Attempt to write at " << address << ", size is " << size_ << endl;
    assert(false);
  }
  data_[address] = value;
}

byte* Memory::getPointer(int address) {
  if (address >= size_) {
    cerr << "Attempt to get a pointer to " << address << ", size is " << size_ << endl;
    assert(false);
  }
  return data_ + address;
}

int Memory::getSize() const {
  return size_;
}
