#include <time.h>
#include <unordered_map>

#include "loader.h"
#include "memory.h"
#include "monitor.h"
#include "vga.h"
#include "x86.h"

using namespace std;

const int kFrameRate = 30;
const int kMemSize = 1 << 20;  // 1 MB

//
// Remake base class. Contains everything but the hook logic.
//
class RemakeBase {
 public:
  RemakeBase() :
      mem_(kMemSize), x86_(&mem_), vga_(&x86_), monitor_(&vga_),
      regs_(*x86_.getRegisters()) {
  }

  void run() {
    next_video_update_ = 0;
    while (true) {
      runHooks();
      x86_.step();

      if (clock() >= next_video_update_) {
        updateMonitor();
      }
    }
  }

  void updateMonitor() {
    monitor_.update();
    next_video_update_ = clock() + (CLOCKS_PER_SEC / kFrameRate);
  }

  virtual void runHooks() = 0;

 protected:
  Memory mem_;
  X86 x86_;
  VGA vga_;
  Monitor monitor_;
  Registers& regs_;


  int next_video_update_;
};


//
// Specialization of Remake that can take pointer-to-member hooks for a derived
// class.
//
template<typename T>
class Remake : public RemakeBase {
 public:

  typedef void(T::*AddressHook)();

  virtual void runHooks() {
    auto it = hooks_.find(x86_.getCS_IP());
    if (it != hooks_.end()) {
      ((T*)this->*(it->second))();
    }
  }

  void addHook(int address, AddressHook hook) {
    hooks_[address] = hook;
  }

 private:
  unordered_map<int, AddressHook> hooks_;
};


//
// Goody remake.
//
class GoodyRemake : public Remake<GoodyRemake> {
 public:
  GoodyRemake() {
    Loader::loadCOM("goody.com", &mem_, &x86_);
    addHook(0x3851, &GoodyRemake::drawCharacter);
    addHook(0x383F, &GoodyRemake::drawTile);
  }

  void drawTile() {
    int tile = regs_.ax;
    int col = regs_.bl;
    int row = regs_.bh;

    cerr << dec;
    cerr << "Draw tile " << tile << " at " << col << ", " << row << endl;
  }

  void drawCharacter() {
    char k = regs_.ah;
    int col = regs_.cl;
    int row = regs_.ch;

    if (k >= 30 && k < 40) {
      k = k - 30 + '0';
    } else if (k > 0) {
      k += 64;
    }

    cerr << dec;
    cerr << "Draw character " << (int)k << "[" << k << "]"
         << " at " << col << ", " << row << endl;
  }

};


int main (int argc, char** argv) {
  GoodyRemake goody;

  try {
    goody.run();
  } catch (const exception& e) {
    goody.updateMonitor();

    cerr << "Exception: " << e.what() << endl;
    int dummy;
    cin >> dummy;
  }

  return 0;
}
