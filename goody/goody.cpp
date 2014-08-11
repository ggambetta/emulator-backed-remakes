#include <time.h>
#include <unordered_map>
#include <memory>

#include "lib/loader.h"
#include "lib/memory.h"
#include "lib/monitor.h"
#include "lib/vga.h"
#include "lib/x86.h"
#include "lib/graphics.h"

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

   // Original screen is 320x200 with 1:1.2 aspect ratio (320x240 1:1)
   // Original game uses 40x25 tiles of 8x8 pixels
   // 1000x750 gives 40x25 tiles of 25x30 (aspect 1:2)
  const int kWindowWidth = 1000;
  const int kWindowHeight = 750;
  const int kTileWidth = 25;
  const int kTileHeight = 30;


  GoodyRemake() {
    Loader::loadCOM("goody.com", &mem_, &x86_);

    addHook(0x3851, &GoodyRemake::drawCharacter);
    addHook(0x383F, &GoodyRemake::drawTile);
    addHook(0x36FF, &GoodyRemake::drawImage);
    addHook(0x3736, &GoodyRemake::drawImage);

    window_.reset(new Window(kWindowWidth, kWindowHeight, "Goody"));
  }

  void drawTile() {
    int tile = regs_.ax;
    int col = regs_.bl;
    int row = regs_.bh;

    /*cerr << dec;
    cerr << "Draw tile " << tile << " at " << col << ", " << row << endl;*/
  }

  void drawImage() {
    int id = regs_.bx;
    int w = regs_.cl * 4;
    int h = regs_.ch;

    int offset = regs_.dx;
    int x = (offset % 80);
    int y = (offset / 80) * 2;

    /*cerr << dec;
    cerr << "Draw image " << id << " (" << w << " x " << h << ") "
         << "at (" << x << ", " << y << ")" << endl; */
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

    /*cerr << dec;
    cerr << "Draw character " << (int)k << "[" << k << "]"
         << " at " << col << ", " << row << endl;*/
  }

  unique_ptr<Window> window_;
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
