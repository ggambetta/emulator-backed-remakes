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
  vector<string> block_comments;
  string code;
  string line_comment;
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


  void flushLine(ostream& os, stringstream& ss) {
    if (!ss.str().empty()) {
      os << ss.str() << endl;
      ss.str("");
    }
  }

  void startLine(stringstream& ss, int address) {
    if (ss.str().empty()) {
      ss << Hex16 << address << "  .DB ";
    }
  }

  void outputBytes(ostream& os, int address, int size) {
    byte* data = mem_->getPointer(address);
    int start = 0;
    stringstream ss;
    while (start < size) {
      startLine(ss, address + start);

      // Find a consecutive group of printable or non-printable characters.
      bool printable = isprint(data[start]); 
      int end = start;
      while (isprint(data[end]) == printable && end < size) {
        end++;
      }

      // Print them.
      if (printable && (end - start) > 3) {
        flushLine(os, ss);
        startLine(ss, address + start);
        ss << "'" << string((char*)data + start, end - start) << "'";
        flushLine(os, ss);
        start = end;
      } else {
        while (start < end && ss.str().size() < 77) {
          ss << Hex8 << (int)data[start++] << ", ";
        }
        if (start < end) {
          flushLine(os, ss);
        }
      }
    }

    flushLine(os, ss);
  }


  void outputAsm(const string& asm_fn) {
    ofstream out(asm_fn);

    int next_address = start_offset_;
    for (const auto& entry: disassembly_) {
      int address = entry.first;
      auto fragment = entry.second;

      // If there's a gap between disassembled chunks, output whatever is in
      // the middle as raw data.
      if (address != next_address) {
        out << endl;
        outputBytes(out, next_address, address - next_address);
      }

      // Space before an address that is jumped to.
      if (entry_points_.count(address) != 0) {
        out << endl;
      }

      // Output block comments.
      if (!fragment->block_comments.empty()) {
        out << endl;
        for (const string& comment : fragment->block_comments) {
          out << "; " << comment << endl;
        }
      }

      // Output disassembly and line comment.
      out << Hex16 << address << "  " << fragment->code;
      if (!fragment->line_comment.empty()) {
        out << "    ; " << fragment->line_comment;
      }
      out << endl;

      next_address = address + fragment->size;
    }

    outputBytes(out, next_address, end_offset_ - next_address); 
  }


  void disassemble() {
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
  }

  bool startsWithAddress(const string& line) {
    if (line.size() < 5) {
      return false;
    }
    return isxdigit(line[0]) && isxdigit(line[1]) &&
           isxdigit(line[2]) && isxdigit(line[3]) &&
           line[4] == ' ';
  }

  void loadConfig(const string& fn) {
    ifstream infile(fn);
    string line;

    while (getline(infile, line)) {
      line = strip(line);
      if (line.empty()) {
        continue;
      }
      vector<string> tokens = split(line);
      string cmd = lower(tokens[0]);
      if (cmd == "entrypoint") {
        if (tokens.size() > 1) {
          addEntryPoint(parseNumber(tokens[1]));
        } else {
          cerr << "Syntax: " << cmd << " <address>" << endl; 
        }
      }
    }
  }

  void mergeComments(const string& asm_fn) {
    ifstream infile(asm_fn);
    string line;

    vector<string> block_comments;
    while (getline(infile, line)) {
      line = strip(line);
      if (line.empty()) {
        continue;
      }

      if (line[0] == ';') {
        // Whole-line comment.
        block_comments.push_back(strip(line.substr(1)));
      } else if (startsWithAddress(line)) {
        // Disassembly line.
        int address = (int)strtol(line.data(), NULL, 16);

        if (disassembly_.count(address) == 0) {
          // Possibly a data address.
          // TODO: Don't lose these comments!
          continue;
        }
        auto fragment = disassembly_[address];

        // Attach block comments.
        if (!block_comments.empty()) {
          fragment->block_comments = block_comments;
          block_comments.clear();
        }

        // Find and attach line comment.
        int idx = line.find(';');
        if (idx == string::npos) {
          continue;
        }
        fragment->line_comment = strip(line.substr(idx + 1));
      }
    }
  }


  void disassembleAndMerge(const string& binary_fn, const string& asm_fn) {
    load(binary_fn);
    disassemble();
    mergeComments(asm_fn);
    outputAsm(asm_fn);
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
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <prefix>" << endl;
    cerr << endl;
    cerr << "Disassembles <prefix>.com into <prefix>.asm." << endl;
    cerr << "If <prefix>.asm exists, attempts to merge the existing comments." << endl;
    cerr << "If <prefix>.cfg exists, reads configuration entries from it:" << endl;
    cerr << "    EntryPoint <address>    Add an explicit entry point to explore." << endl;

    return 1;
  }

  Memory mem(1 << 20);  // 1 MB
  X86Disassembler x86(&mem);

  string prefix = argv[1];
  x86.loadConfig(prefix + ".cfg");
  x86.disassembleAndMerge(prefix + ".com", prefix + ".asm");

  return 0;
}
