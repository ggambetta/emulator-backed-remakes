#include <iostream>
#include <fstream>
#include <vector>
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
    error_ = false;
    screenshot_ = 0;
    instance_ = this;

    signal(SIGINT, &Runner::catchSignal);
    signal(SIGABRT, &Runner::catchSignal);
  }


  void saveScreenshot() {
    stringstream ss;
    ss << "screenshot" << setw(3) << setfill('0') << screenshot_++ << ".ppm";
    vga_->save(ss.str().data());
  }


  void run() {
    string last_command;
    while (true) {
      if (!x86_->isExecutePending()) {
        x86_->fetchAndDecode();
      }
      x86_->outputCurrentOperation(cout);

      cout << ">>> ";
      string command;
      getline(cin, command);

      if (cin.eof()) {
        break;
      }

      if (command.empty()) {
        command = last_command;
      }

      executeCommand(command);

      last_command = command;
    }
  }


  static void catchSignal (int signal) {
    instance_->handleSignal(signal);
  }

  void handleSignal(int signal) {
    if (signal == 6) {
      error_ = true;
      cerr << "ABRT" << endl;
    } else if (signal == 2) {
      error_ = true;
      cout << endl;
    } else {
      cerr << "Got signal " << signal << endl;
      exit(1);
    }
  }


  void doRun() {
    doStep(-1);
  }

  void doStep(int steps) {
    while ((steps == -1 || steps--) && !error_) {
      if (!x86_->isExecutePending()) {
        x86_->fetchAndDecode();
      }
      x86_->execute();
    }
  }

  void doSkip() {
    x86_->getRegisters()->ip += x86_->getBytesFetched(); 
    x86_->clearExecutionState();
  }


  void executeCommand(const string& command) {
    error_ = false;

    vector<string> tokens = split(command);
    if (tokens.empty()) {
      tokens.push_back("step");
    }

    try {
      string action = tokens[0];
      if (action == "s" || action == "step") {
        int steps = 1;
        if (tokens.size() > 1) {
          steps = stoi(tokens[1]);
        }
        doStep(steps);
      } else if (action == "r" || action == "run") {
        doRun();
      } else if (action == "skip") {
        doSkip();
      } else {
        cerr << "Unknown command '" << action << "'" << endl;
      }
    } catch(const runtime_error& e) {
      cerr << "ERROR: " << e.what() << endl;
    }
  }

 private:
  X86* x86_;
  VGA* vga_;

  bool error_;
  int screenshot_;

  static Runner* instance_;
};

Runner* Runner::instance_ = nullptr;



int main (int argc, char** argv) {
  Memory mem(1 << 20);  // 1 MB
  X86 x86(&mem);
  VGA vga(&x86);

  Loader::loadCOM("goody.com", &x86);

  Runner runner(&x86, &vga);
  runner.run();

  cout << endl;

  return 0;
}
