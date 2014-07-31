#include <iostream>
#include <fstream>
#include <cassert>
#include <iomanip>
#include <sstream>
#include <signal.h>

#include "x86.h"
#include "vga.h"
#include "memory.h"
#include "loader.h"

using namespace std;

class Runner {
 public:
  Runner (X86* x86, VGA* vga) : x86_(x86), vga_(vga) {
    running_ = false;
    screenshot_ = 0;

    signal(SIGINT, &Runner::handleSignal);
    signal(SIGABRT, &Runner::handleSignal);
  }

  void saveScreenshot() {
    stringstream ss;
    ss << "screenshot" << setw(3) << setfill('0') << screenshot_++ << ".ppm";
    vga_->save(ss.str().data());
  }

  void run() {
    while (true) {
      x86_->step();
    }
  }

  static void handleSignal (int signal) {
    cerr << "Got signal " << signal << endl;
    exit(1);
  }

 private:
  X86* x86_;
  VGA* vga_;

  bool running_;
  int screenshot_;
};



int main (int argc, char** argv) {
  Memory mem(1 << 20);  // 1 MB
  X86 x86(&mem);
  VGA vga(&x86);

  Loader::loadCOM("goody.com", &x86);

  x86.setDebugLevel(1);

  Runner runner(&x86, &vga);
  runner.run();

  /*Loader::loadCOM("goody.com", &x86);

  int screenshot = 0;
  int steps = 0;
  while (true) {
    x86.step();
    steps++;

    if (steps == 1000) {
      steps = 0;
      clog << "BREAKPOINT" << endl;

    }
  }*/

  return 0;
}
