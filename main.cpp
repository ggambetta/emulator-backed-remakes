#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <vector>

#include "loader.h"
#include "memory.h"
#include "monitor.h"
#include "vga.h"
#include "x86.h"

using namespace std;

const char kPrompt[] = ">>> ";

class Runner {
 public:
  Runner (X86* x86, Monitor* monitor) : x86_(x86), monitor_(monitor) {
    error_ = false;
    instance_ = this;
    running_ = false;

    signal(SIGINT, &Runner::catchSignal);
    signal(SIGABRT, &Runner::catchSignal);
  }


  void run() {
    string last_command;
    while (true) {
      if (!x86_->isExecutePending()) {
        x86_->fetchAndDecode();
      }
      x86_->outputCurrentOperation(cout);

      cout << kPrompt;
      string command;
      getline(cin, command);

      if (cin.eof()) {
        break;
      }

      if (command.empty()) {
        command = last_command;
      }
      last_command = command;

      executeCommand(command);
      monitor_->update();
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

      if (!running_) {
        cerr << endl << "Press ^D to quit." << endl;
        cerr << kPrompt;
      }
    } else {
      cerr << "Got signal " << signal << endl;
      exit(1);
    }
  }


  void doRun() {
    doStep(-1);
  }

  void doStep(int steps) {
    running_ = true;
    while ((steps == -1 || steps--) && !error_) {
      if (!x86_->isExecutePending()) {
        x86_->fetchAndDecode();
      }
      x86_->execute();
    }
    running_ = false;
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
      } else if (action == "ss") {
        if (tokens.size() > 1) {
          monitor_->saveToFile(tokens[1]);
        } else {
          cerr << "Syntax: " << action << " <filename>" << endl;
        }
      } else if (action == "scale") {
        if (tokens.size() > 1) {
          monitor_->setScale(stoi(tokens[1]));
        } else {
          cerr << "Syntax: " << action << " <scale>" << endl;
        }
      } else {
        cerr << "Unknown command '" << action << "'" << endl;
      }
    } catch(const runtime_error& e) {
      cerr << "ERROR: " << e.what() << endl;
    }
    cerr << endl;
  }

 private:
  X86* x86_;
  Monitor* monitor_;

  bool error_;
  bool running_;

  static Runner* instance_;
};

Runner* Runner::instance_ = nullptr;



int main (int argc, char** argv) {
  Memory mem(1 << 20);  // 1 MB
  X86 x86(&mem);
  VGA vga(&x86);
  Monitor monitor(&vga);

  Loader::loadCOM("goody.com", &x86);

  Runner runner(&x86, &monitor);
  runner.run();

  cout << endl;

  return 0;
}
