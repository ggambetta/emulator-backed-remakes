#ifndef __LOADER_H__
#define __LOADER_H__

#include <string>

class X86;

class Loader {
 public:
  static void loadCOM(const std::string& filename, X86* x86, int& start, int& end);
};

#endif  // __LOADER_H__
