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
#include "x86_base.h"

using namespace std;

struct Fragment {
  int size;
  string code;
};


class X86Disassembler : public X86Base {
 public:
  X86Disassembler(Memory* mem) : mem_(mem) {
  }

  // ----------------------------------------
  //  X86Base overrides.
  // ----------------------------------------
  virtual byte fetch() override {
    size_++;
    return mem_->read(ip_++);
  }

  virtual void CALL_w() override { addEntryPoint(*warg1); }

  virtual void LOOP() override { addEntryPoint(*warg1); }
  virtual void LOOPNZ() override { addEntryPoint(*warg1); }
  virtual void LOOPZ() override { addEntryPoint(*warg1); }

  virtual void JA() override { addEntryPoint(*warg1); }
  virtual void JB() override { addEntryPoint(*warg1); }
  virtual void JBE() override { addEntryPoint(*warg1); }
  virtual void JCXZ() override { addEntryPoint(*warg1); }
  virtual void JG() override { addEntryPoint(*warg1); }
  virtual void JGE() override { addEntryPoint(*warg1); }
  virtual void JL() override { addEntryPoint(*warg1); }
  virtual void JLE() override { addEntryPoint(*warg1); }
  virtual void JNB() override { addEntryPoint(*warg1); }
  virtual void JNO() override { addEntryPoint(*warg1); }
  virtual void JNS() override { addEntryPoint(*warg1); }
  virtual void JNZ() override { addEntryPoint(*warg1); }
  virtual void JO() override { addEntryPoint(*warg1); }
  virtual void JPE() override { addEntryPoint(*warg1); }
  virtual void JPO() override { addEntryPoint(*warg1); }
  virtual void JS() override { addEntryPoint(*warg1); }
  virtual void JZ() override { addEntryPoint(*warg1); }

  virtual void JMP_b() override { addEntryPoint(*warg1); running_ = false; }
  virtual void JMP_p() override { addEntryPoint(*warg1); running_ = false; }

  // Ignore JMP <reg>
  virtual void JMP_w() override { if (*warg1) { addEntryPoint(*warg1); } running_ = false; }

  virtual void RET() override { running_ = false; }
  virtual void IRET() override { running_ = false; }
  virtual void RETF() override { running_ = false; }
  virtual void RETF_w() override { running_ = false; }
  virtual void RET_w() override { running_ = false; }
  
  virtual void notImplemented(const char* opcode_name) override {
  }
  
  virtual void invalidOpcode() override {
    stringstream ss;
    ss << "Invalid opcode 0x" << Hex8 << (int)opcode_;
    FATAL(ss.str());
  }

  virtual word* getReg16Ptr(int reg) override {
    if (reg == X86Base::R16_IP) {
      return &ip_;
    }
    dummy_ = 0;
    return &dummy_;
  }
  

  virtual byte* getReg8Ptr(int reg) override {
    dummy_ = 0;
    return (byte*)&dummy_;
  }

  virtual word* getMem16Ptr(word segment, word offset) override {
    return (word*)mem_->getPointer((segment << 4) + offset);
  }

  virtual bool getFlag(word mask) const override {
    return false;
  }
  // ----------------------------------------

  void load(const string& filename) {
    Loader::loadCOM(filename, mem_, this, start_offset_, end_offset_);
    addEntryPoint(start_offset_);
  }

  void addEntryPoint(int address) {
    if (entry_points_.count(address) == 0) {
      //cout << "Found new entry point: " << Hex16 << address << "h [from " << Hex16 << ip_ << "]" <<  endl;
      entry_points_[address] = false;
    }
  }

  void explore(int address) {
    //cout << "Starting disassembly at " << Hex16 << address << endl;
    ip_ = address;
    running_ = true;
    while(running_) {
      clearExecutionState();
      size_ = 0;

      int address = ip_;
      fetchAndDecode();

      if (disassembly_.count(address) == 0) {
        shared_ptr<Fragment> fragment(new Fragment());
        fragment->size = size_;
        fragment->code = getOpcodeDesc();
        disassembly_[address] = fragment;
      }

      rep_opcode_ = 0;
      execute();
    }
  }

  void disassembleBytes(ostream& os, byte* data, int size) {
    if (size == 0) {
      return;
    }
    os << endl;
    os << ".DB (" << dec << size << ")" << endl;
  }

  void disassemble(const string& binary_fn, const string& asm_fn) {
    // Load the binary.
    load(binary_fn);

    // Disassemble.
    bool found;
    do {
      found = false;
      for (auto& kv: entry_points_) {
        if (!kv.second) {
          kv.second = true;
          found = true;
          explore(kv.first);
          continue;
        }
      }
    } while (found);

    // Output.
    ofstream out(asm_fn);

    byte* ram = mem_->getPointer(0);
    int next_address = start_offset_;
    for (const auto& entry: disassembly_) {
      int address = entry.first;
      auto fragment = entry.second;

      if (address != next_address || address == start_offset_) {
        disassembleBytes(out, ram + address, address - next_address);
        out << endl;
      }

      out << Hex16 << address << "        " << fragment->code << endl;
      next_address = address + fragment->size;
    }

    disassembleBytes(out, ram + next_address, end_offset_ - next_address); 
  }


 private:
  map<int, bool> entry_points_;

  Memory* mem_;
  word dummy_;

  bool running_;
  int size_;

  word ip_;

  int start_offset_;
  int end_offset_;

  map<int, shared_ptr<Fragment>> disassembly_;
};


int main (int argc, char** argv) {
  Memory mem(1 << 20);  // 1 MB
  X86Disassembler x86(&mem);
  x86.disassemble("goody.com", "goody.asm");

  cout << endl;
  return 0;
}
