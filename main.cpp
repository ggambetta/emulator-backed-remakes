#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
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
  Runner (X86* x86, VGA* vga, Monitor* monitor) : x86_(x86), vga_(vga), monitor_(monitor) {
    error_ = false;
    instance_ = this;
    running_ = false;

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
    while ((steps == -1 || steps--) && !error_) {
      int address = x86_->getCS_IP();

      if (!x86_->isExecutePending()) {
        x86_->fetchAndDecode();
      }
      addToDisassembly();

      if (breakpoints_.find(address) != breakpoints_.end()) {
        cout << "Breakpoint." << endl;
        break;
      }

      x86_->execute();
    }
    running_ = false;
  }


  void addToDisassembly() {
    if (!disassemble_) {
      return;
    }

    int size = x86_->getBytesFetched();
    int address = x86_->getCS_IP() - size;
    if (disassembly_.find(address) != disassembly_.end()) {
      // TODO: If the disassembly is different, code if self-modifying.
      // Think what to do with this.
      return;
    }
    disassembly_.insert({address, {size, x86_->getOpcodeDesc()}});
  }

  void doSkip() {
    x86_->getRegisters()->ip += x86_->getBytesFetched(); 
    x86_->clearExecutionState();
  }


  void doLoad(const string& filename) {
    Loader::loadCOM(filename, x86_, start_offset_, end_offset_);
    cout << "File loaded, [" << Hex16 << start_offset_ << " - " 
      << Hex16 << end_offset_ << "]" << endl;
    disassembly_.clear();
  }


  void disassembleBytes(ostream& os, byte* data, int size) {
    if (size == 0) {
      return;
    }
    os << endl;
    os << ".DB (" << dec << size << ")" << endl;
  }


  void doSaveASM(ostream& os) {
    byte* ram = x86_->getMemory()->getPointer(0);

    int next_address = start_offset_;
    for (const auto& entry: disassembly_) {
      int address = entry.first;
      int size = entry.second.first;
      const string& source = entry.second.second;

      if (address != next_address || address == start_offset_) {
        disassembleBytes(os, ram + address, address - next_address);
        os << endl;
        os << ".ORG " << Hex16 << address << "h" << endl;
      }

      os << "        " << source << endl;

      next_address = address + size;
    }

    disassembleBytes(os, ram + next_address, end_offset_ - next_address); 
  }


  void doSaveASM(const string& filename) {
    ofstream file(filename);
    doSaveASM(file);
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


  void doBreak(const string& addr_string) {
    int addr = parseNumber(addr_string);
    if (breakpoints_.find(addr) != breakpoints_.end()) {
      cerr << "Breakpoint at " << addr_string << " already set." << endl;
    } else {
      breakpoints_.insert(addr);
      cout << "Set breakpoint at " << addr_string << "." << endl;
    }
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
      } else if (action == "load") {
        if (tokens.size() > 1) {
          doLoad(tokens[1]);
        } else {
          cerr << "Syntax: " << action << " <filename>" << endl;
        }
      } else if (action == "saveasm") {
        if (tokens.size() > 1) {
          doSaveASM(tokens[1]);
        } else {
          doSaveASM(cout);
        }
      } else if (action == "state") {
        doState();
      } else if (action == "break") {
        if (tokens.size() > 1) {
          doBreak(tokens[1]);
        } else {
          cerr << "Syntax: " << action << " <address>" << endl;
        }
      } else if (action == "disassemble") {
        disassemble_ = true;
        if (tokens.size() > 1) {
          disassemble_ = parseBool(tokens[1]);
        }
        cout << "Disassembly " << (disassemble_ ? "en" : "dis") << "abled." << endl;
      } else if (action == "set") {
        if (tokens.size() > 2) {
          doSet(tokens[1], tokens[2]);
        } else {
          cerr << "Syntax: " << action << " <register> <value>" << endl;
        }
      } else if (action == "reset") {
        x86_->reset();
        vga_->setVideoMode(0);
        breakpoints_.clear();
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

  bool disassemble_;

  // Start and end offset of the loaded binary.
  int start_offset_, end_offset_;

  // Map of { start => size, disassembly }.
  map<int, pair<int, string>> disassembly_;

  // Breakpoints.
  set<int> breakpoints_;

  static Runner* instance_;
};

Runner* Runner::instance_ = nullptr;



int main (int argc, char** argv) {
  Memory mem(1 << 20);  // 1 MB
  X86 x86(&mem);
  VGA vga(&x86);
  Monitor monitor(&vga);

  Runner runner(&x86, &vga, &monitor);
  runner.runScript("startup.cmd");
  runner.run();

  cout << endl;

  return 0;
}
