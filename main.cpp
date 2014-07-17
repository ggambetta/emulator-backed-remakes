#include <iostream>
#include <fstream>
#include <cassert>
#include <iomanip>

#include "x86.h"
#include "memory.h"

using namespace std;

static const int kMemSize = 2 << 16;
static const int kCOMOffset = 0x0100;

//
// Runner.
//
class Runner {
 public:
  Runner()
      : mem_(kMemSize), x86_(&mem_) {
    regs_ = x86_.getRegisters();
    x86_.setDebugLevel(0);
  }

  void load(const string& filename) {
    ifstream file(filename, ios::in | ios::binary | ios::ate);
    int size = (int)file.tellg();
    cout << "File is "<< size << " bytes long." << endl;

    assert(kCOMOffset + size < kMemSize);

    file.seekg(0, ios::beg);
    file.read((char*)mem_.getPointer(kCOMOffset), size);

    regs_->cs = regs_->ds = regs_->es = regs_->ss = 0;
    regs_->ip = kCOMOffset;

    regs_->ss = 0;
    regs_->sp = 0xFFFF;
    
    cout << "File successfully loaded." << endl;
  }

  void run() {
    while (true) {
      x86_.step();
    }
  }


 private:
  Memory mem_;
  X86 x86_;
  Registers* regs_;
};


int main (int argc, char** argv) {
  Runner runner;

  runner.load("goody.com");
  runner.run();

  return 0;
}
