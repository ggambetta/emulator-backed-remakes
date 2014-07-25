#include <iostream>
#include <fstream>
#include <cassert>
#include <iomanip>
#include <sstream>

#include "x86.h"
#include "vga.h"
#include "memory.h"
#include "loader.h"

using namespace std;

static const int kMemSize = 2 << 20;  // 1 MB

//
// Runner.
//
class Runner {
 public:
  Runner()
      : mem_(kMemSize), x86_(&mem_), vga_(&x86_) {
    x86_.setDebugLevel(0);
  }

  void load(const string& filename) {
    Loader::loadCOM(filename, &x86_);
  }

  void run() {
    while (true) {
      x86_.step();
  
     if (x86_.getCS_IP() == 0x3735) {
        clog << "BREAKPOINT" << endl;

        static int idx = 0;
        stringstream ss;
        ss << "screenshot" << setw(3) << setfill('0') << idx++ << ".ppm";
        vga_.save(ss.str().data());
      }
    }
  }


 private:
  Memory mem_;
  X86 x86_;
  VGA vga_;
};


int main (int argc, char** argv) {
  Runner runner;

  runner.load("goody.com");
  runner.run();

  return 0;
}
