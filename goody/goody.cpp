// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#include <iomanip>
#include <memory>
#include <ctime>
#include <sstream>
#include <unordered_map>

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

  virtual void updateMonitor() {
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
typedef unordered_map<int, shared_ptr<Image>> ImageMap;

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

    addHook(0x36F3, &GoodyRemake::drawUI);
    addHook(0x3851, &GoodyRemake::drawGlyph);
    addHook(0x383F, &GoodyRemake::drawTile);

    window_.reset(new Window(kWindowWidth, kWindowHeight, "Goody"));
  }

  void drawGlyph() {
    char k = regs_.ah;
    int col = regs_.cl;
    int row = regs_.ch;

    Image* glyph = getGlyphImage(k);
    if (glyph) {
      window_->drawImage(glyph, col*kTileWidth, row*kTileHeight); 
    }
  }

  void drawUI() {
    shared_ptr<Image> ui(new Image("assets/ui.png"));
    window_->drawImage(ui.get(), 0, 600);
  }

  void drawTile() {
    int tile_id = regs_.ax;
    int col = regs_.bl;
    int row = regs_.bh;

    Image* tile = getTileImage(tile_id);
    if (tile) {
      window_->drawImage(tile, col*kTileWidth, row*kTileHeight); 
    }
  }

  virtual void updateMonitor() {
    Remake<GoodyRemake>::updateMonitor();
    window_->update();
  }


  Image* getOrLoad(int id, const string& prefix, ImageMap& cache) {
    if (cache.count(id) == 0) {
      Image* tile = nullptr;
      stringstream ss;
      ss << prefix << dec << setw(3) << setfill('0') << id << ".png";
      if (fileExists(ss.str())) {
        tile = new Image(ss.str());
      }
      cache[id].reset(tile);
    }
    return cache[id].get();
  }


  Image* getTileImage(int id) {
    return getOrLoad(id, "assets/tile_", tiles_);
  }


  Image* getGlyphImage(int id) {
    return getOrLoad(id, "assets/glyph_", glyphs_);
  }



  unique_ptr<Window> window_;
  ImageMap tiles_;
  ImageMap glyphs_;
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
