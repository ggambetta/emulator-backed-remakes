// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "lib/loader.h"
#include "lib/memory.h"
#include "lib/monitor.h"
#include "lib/vga.h"
#include "lib/x86.h"

using namespace std;

const char kPrompt[] = ">>> ";
const int kFrameRate = 30;

class Runner {
 public:
  Runner (X86* x86, VGA* vga, Monitor* monitor) : x86_(x86), vga_(vga), monitor_(monitor) {
    error_ = false;
    instance_ = this;
    running_ = false;
    breakpoint_once_ = -1;

    signal(SIGINT, &Runner::catchSignal);
    signal(SIGABRT, &Runner::catchSignal);
  }

  void runScript(const string& filename) {
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
      executeCommand(line);
    }
  }

  void run() {
    string last_command;
    while (true) {
      if (!x86_->isExecutePending()) {
        fetched_address_ = x86_->getCS_IP();
        x86_->fetchAndDecode();
      }
      x86_->outputCurrentOperation(cout);

      cout << endl << kPrompt;
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
    bool first = true;

    int next_video_update = 0;

    while ((steps == -1 || steps--) && !error_) {
      if (!x86_->isExecutePending()) {
        fetched_address_ = x86_->getCS_IP();
        x86_->fetchAndDecode();
      }

      // Handle breakpoints.
      if (fetched_address_ == breakpoint_once_) {
        break;
      }
      if (!first && breakpoints_.count(fetched_address_) != 0) {
        cout << "Breakpoint." << endl;
        break;
      }

      x86_->execute();
      first = false;

      if (clock() >= next_video_update) {
        monitor_->update();
        next_video_update = clock() + (CLOCKS_PER_SEC / kFrameRate);
      }
    }
    breakpoint_once_ = -1;
    running_ = false;
  }


  void doSkip() {
    x86_->getRegisters()->ip += x86_->getBytesFetched(); 
    x86_->clearExecutionState();
  }


  void doLoad(const string& filename) {
    x86_->clearExecutionState();
    Loader::loadCOM(filename, x86_->getMemory(), x86_, start_offset_, end_offset_);
    cout << "File loaded, [" << Hex16 << start_offset_ << " - " 
      << Hex16 << end_offset_ << "]" << endl;
  }


  void doState() {
    Registers* regs = x86_->getRegisters();
    cout << "AX " << Hex16 << regs->ax << "  "
         << "BX " << Hex16 << regs->bx << "  "
         << "CX " << Hex16 << regs->cx << "  "
         << "DX " << Hex16 << regs->dx << endl;

    cout << "CS " << Hex16 << regs->cs << "  "
         << "SS " << Hex16 << regs->ss << "  "
         << "DS " << Hex16 << regs->ds << "  "
         << "ES " << Hex16 << regs->es << endl;

    cout << "BP " << Hex16 << regs->bp << "  "
         << "SP " << Hex16 << regs->sp << "  "
         << "DI " << Hex16 << regs->di << "  "
         << "SI " << Hex16 << regs->si << endl;
    
    cout << "IP " << Hex16 << regs->ip - x86_->getBytesFetched() << "  "
         << "FLAGS ";
  
    for (int i = 15; i >= 0; i--) {
      int mask = i << 15;
      if (regs->flags & mask) {
        cout << FLAG_NAME[i]; 
      } else {
        cout << "-";
      }
    }
    cout << endl;

    cout << endl;
  }

  void doUntil(const string& addr_string) {
    breakpoint_once_ = parseNumber(addr_string);
    cout << "Running until " << Hex16 << breakpoint_once_ << "h" << endl; 
    doRun();
  }

  void doBreak(const string& addr_string) {
    int addr = parseNumber(addr_string);
    if (breakpoints_.find(addr) != breakpoints_.end()) {
      breakpoints_.erase(addr);
      cout << "Removed breakpoint at " << Hex16 << addr << "h." << endl;
    } else {
      breakpoints_.insert(addr);
      cout << "Set breakpoint at " << Hex16 << addr << "h." << endl;
    }
  }

  void doOver() {
    breakpoint_once_ = x86_->getCS_IP();
    doRun();
  }

  void doPoke(const string& addr_string, const string& val_str) {
    int addr = parseNumber(addr_string);
    int value = parseNumber(val_str);
    x86_->getMemory()->write(addr, value);
    x86_->refetch();
  }

  void doSet(const string& reg, const string& val_str) {
    string ureg = upper(reg);
    int value = parseNumber(val_str);

    for (int i = 0; i < X86::R16_COUNT; i++) {
      if (ureg == REG16_DESC[i]) {
        *x86_->getReg16Ptr(i) = value;    
        cout << ureg << " = " << Hex16 << value << "h" << endl;
      }
    }

    for (int i = 0; i < X86::R8_COUNT; i++) {
      if (ureg == REG8_DESC[i]) {
        *x86_->getReg8Ptr(i) = value & 0xFF;
        cout << ureg << " = " << Hex8 << (int)(value & 0xFF) << "h" << endl;
      }
    }
  }

  void doCallStack() {
    auto call_stack = x86_->getCallStack();
    for (const auto& csip : call_stack) {
      cout << Addr(csip.first, csip.second) << endl; 
    }
    cout << endl;
  }

  void doEntryPoints() {
    auto entry_points = x86_->getEntryPoints();
    for (int address : entry_points) {
      cout << "EntryPoint " << Hex16 << address << "h" << endl;
    }
    cout << endl;
  }


  void executeCommand(const string& command) {
    error_ = false;

    vector<string> tokens = split(command);
    if (tokens.empty()) {
      return;
    }

    try {
      string action = tokens[0];
      if (action == "s" || action == "step") {
        // STEP [count] - execute one or more instructions.
        int steps = 1;
        if (tokens.size() > 1) {
          steps = stoi(tokens[1]);
        }
        doStep(steps);
      } else if (action == "r" || action == "run") {
        // RUN - run until stopped.
        doRun();
      } else if (action == "skip") {
        // SKIP - skip over the current instruction.
        doSkip();
      } else if (action == "ss" || action == "screenshot") {
        // SCREENSHOT <filename> - save the emulated screen as PPM.
        if (tokens.size() > 1) {
          monitor_->savePPM(tokens[1]);
        } else {
          cerr << "Syntax: " << action << " <filename>" << endl;
        }
      } else if (action == "scale") {
        // SCALE <scale> - set the emulated screen scale.
        if (tokens.size() > 1) {
          monitor_->setScale(stoi(tokens[1]));
        } else {
          cerr << "Syntax: " << action << " <scale>" << endl;
        }
      } else if (action == "load") {
        // LOAD <filename> - load a COM file.
        if (tokens.size() > 1) {
          doLoad(tokens[1]);
        } else {
          cerr << "Syntax: " << action << " <filename>" << endl;
        }
      } else if (action == "state") {
        // STATE - print the state of the registers.
        doState();
      } else if (action == "break") {
        // BREAK <address> - add/remove a permanent breakpoint at the given address.
        if (tokens.size() > 1) {
          doBreak(tokens[1]);
        } else {
          cerr << "Syntax: " << action << " <address>" << endl;
        }
      } else if (action == "poke") {
        // POKE <address> <value> - change a memory address.
        if (tokens.size() > 2) {
          doPoke(tokens[1], tokens[2]);
        } else {
          cerr << "Syntax: " << action << " <address> <value>" << endl;
        }
      } else if (action == "set") {
        // SET <register> <value> - set the value of a register.
        if (tokens.size() > 2) {
          doSet(tokens[1], tokens[2]);
        } else {
          cerr << "Syntax: " << action << " <register> <value>" << endl;
        }
      } else if (action == "reset") {
        // RESET - restart and clear breakpoints.
        x86_->reset();
        vga_->setVideoMode(0);
        breakpoints_.clear();
      } else if (action == "over") {
        // OVER - runs until the instruction following the current one
        // in memory. Mostly equivalent to STEP, except for CALL and the
        // like; STEP would step into the CALL, OVER stops after the CALL
        // returns.
        doOver();
      } else if (action == "until") {
        // UNTIL <address> - add a one-time breakpoint at the given address.
        if (tokens.size() > 1) {
          doUntil(tokens[1]);
        } else {
          cerr << "Syntax: " << action << " <address>" << endl;
        }
      } else if (action == "cs" || action == "callstack") {
        // CALLSTACK - print the current call stack.
        doCallStack();
      } else if (action == "cvram") {
        // CVRAM - clear VRAM.
        vga_->clearVRAM();
      } else if (action == "rvram") {
        // CVRAM - randomize VRAM.
        vga_->randomVRAM();
      } else if (action == "ep" || action == "entrypoints") {
        // ENTRYPOINTS - print all collected entry points in a format suitable
        // for the disassembler .cfg.
        doEntryPoints();
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
  Monitor* monitor_;

  bool error_;
  bool running_;

  // Start and end offset of the loaded binary.
  int start_offset_, end_offset_;

  // Address of last fetched instruction.
  int fetched_address_;

  // Breakpoints.
  unordered_set<int> breakpoints_;
  int breakpoint_once_;

  static Runner* instance_;
};

Runner* Runner::instance_ = nullptr;


int main (int argc, char** argv) {
  Memory mem(1 << 20);  // 1 MB
  X86 x86(&mem);
  VGA vga(&x86);
  Monitor monitor(&vga);

  Runner runner(&x86, &vga, &monitor);
  runner.runScript("runner.cmd");
  runner.run();

  cout << endl;

  return 0;
}
