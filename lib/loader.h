// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#ifndef __LOADER_H__
#define __LOADER_H__

#include <string>

class Memory;
class X86Base;

class Loader {
 public:
  static void loadCOM(const std::string& filename, Memory* mem, X86Base* x86);
  static void loadCOM(const std::string& filename, Memory* mem, X86Base* x86, int& start, int& end);
};

#endif  // __LOADER_H__
