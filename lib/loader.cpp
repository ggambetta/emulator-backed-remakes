// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#include "loader.h"
#include "x86.h"
#include "memory.h"

#include <fstream>

using namespace std;

static const int kCOMOffset = 0x0100;


void Loader::loadCOM(const string& filename, Memory* mem, X86Base* x86) {
  int dummy;
  loadCOM(filename, mem, x86, dummy, dummy);
}


void Loader::loadCOM(const string& filename, Memory* mem, X86Base* x86,
                     int& start, int& end) {
  ifstream file(filename, ios::in | ios::binary | ios::ate);
  int size = (int)file.tellg();
  ASSERT(kCOMOffset + size < mem->getSize());

  start = kCOMOffset;
  end = size;

  file.seekg(0, ios::beg);
  file.read((char*)mem->getPointer(kCOMOffset), size);

  *x86->getReg16Ptr(X86::R16_CS) = 0;
  *x86->getReg16Ptr(X86::R16_DS) = 0;
  *x86->getReg16Ptr(X86::R16_ES) = 0;

  *x86->getReg16Ptr(X86::R16_SS) = 0;
  *x86->getReg16Ptr(X86::R16_SP) = 0xFFFF;

  *x86->getReg16Ptr(X86::R16_IP) = kCOMOffset;
}
