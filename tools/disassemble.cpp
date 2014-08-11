#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <signal.h>
#include <sstream>
#include <vector>

#include "lib/loader.h"
#include "lib/memory.h"
#include "lib/x86_base.h"

using namespace std;

int kMaxInstructionSize = 6;

struct Fragment {
  enum {
    CODE,
    DATA,
  };
  int type;

  int size;
  vector<string> block_comments;
  string code;
  string line_comment;
};


struct EntryPoint {
  bool explored;

  enum {
    CALL,
    JUMP,
    MANUAL,
  };
  int type;
};


class X86Disassembler : public X86Base {
 public:
  X86Disassembler(Memory* mem) : mem_(mem) {
    dump_raw_ = false;
  }

  // ----------------------------------------
  //  X86Base overrides.
  // ----------------------------------------
  virtual byte fetch() override {
    size_++;
    return mem_->read(ip_++);
  }

  virtual void CALL_w() override { addEntryPoint(*warg1, EntryPoint::CALL); }

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

  virtual byte* getMem8Ptr(word segment, word offset) override {
    return mem_->getPointer((segment << 4) + offset);
  }

  virtual bool getFlag(word mask) const override {
    return false;
  }
  // ----------------------------------------

  void load(const string& filename) {
    Loader::loadCOM(filename, mem_, this, start_offset_, end_offset_);
    addEntryPoint(start_offset_);
  }

  void addEntryPoint(int address, int type = EntryPoint::JUMP) {
    if (entry_points_.count(address) != 0) {
      return;
    }

    //cout << "Found new entry point: " << Hex16 << address << "h [from " << Hex16 << ip_ << "]" <<  endl;
    EntryPoint* ep = new EntryPoint();
    ep->explored = false;
    ep->type = type;
    entry_points_[address].reset(ep);
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
        fragment->type = Fragment::CODE;
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

  void outputDataFragment(ostream& os, int address, Fragment* fragment) {
    // Blank line before data dump.
    if (fragment->block_comments.empty()) {
      os << endl;
    }

    byte* data = mem_->getPointer(address);
    int start = 0;
    stringstream ss;
    while (start < fragment->size) {
      startLine(ss, address + start);

      // Find a consecutive group of printable or non-printable characters.
      bool printable = isprint(data[start]); 
      int end = start;
      while (isprint(data[end]) == printable && end < fragment->size) {
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


  void outputRawBytes(ostream& os, int address, int size) {
    byte* raw = mem_->getPointer(address);
    while (size--) {
      os << Hex8 << (int)(*raw++);
    }
  }

  void outputComment(ostream& os, const string& comment) {
    os << "; " << comment << endl;
  }


  void outputCodeFragment(ostream& os, int address, Fragment* fragment) {
    // Space and synthetic comments before "interesting" addresses.
    EntryPoint* ep = entry_points_[address].get();
    if (ep && fragment->block_comments.empty()) {
      os << endl;
      if (ep->type == EntryPoint::CALL) {
        outputComment(os, hex16ToString(address) + "h");
      }
    }

    // Address.
    os << Hex16 << address << "  ";

    // Raw bytes.
    if (dump_raw_) {
      outputRawBytes(os, address, fragment->size);
      os << string((kMaxInstructionSize - fragment->size) * 2, ' ') << "  ";
    }

    // Disassembly.
    os << fragment->code;

    // Line comment.
    if (!fragment->line_comment.empty()) {
      os << "    ; " << fragment->line_comment;
    }

    os << endl;
  }

  void outputAsm(const string& asm_fn) {
    ofstream out(asm_fn);

    for (const auto& entry: disassembly_) {
      int address = entry.first;
      auto fragment = entry.second.get();

      // Output block comments.
      if (!fragment->block_comments.empty()) {
        out << endl;
        for (const string& comment : fragment->block_comments) {
          outputComment(out, comment);
        }
      }

      // Output the fragment contents.
      if (fragment->type == Fragment::CODE) {
        outputCodeFragment(out, address, fragment);
      } else if (fragment->type == Fragment::DATA) {
        outputDataFragment(out, address, fragment);
      }
    }
  }


  void disassemble() {
    exploreEntryPoints();
    addDataFragments();
    verifyCoverage();
  }

  void exploreEntryPoints() {
    bool found;
    do {
      found = false;
      for (auto& kv: entry_points_) {
        if (!kv.second->explored) {
          kv.second->explored = true;
          found = true;
          explore(kv.first);
          continue;
        }
      }
    } while (found);
  }

  // Verifies every byte between start_offset_ and end_offset_ is covered
  // by exactly one fragment.
  void verifyCoverage() {
    int next_address = start_offset_;
    for (const auto& entry: disassembly_) {
      int address = entry.first;
      ASSERT(address == next_address);
      next_address = address + entry.second->size;
    }
    ASSERT(next_address = end_offset_);
  }


  void coverAddressGap(int begin, int end) {
    if (begin == end) {
      return;
    }

    Fragment* fragment = new Fragment();
    fragment->type = Fragment::DATA;
    fragment->size = end - begin;

    disassembly_[begin].reset(fragment);
  }


  // Ensures every address between start_offset_ and end_offset_ is covered
  // by some CODE or DATA fragment.
  void addDataFragments() {
    int next_address = start_offset_;
    for (const auto& entry: disassembly_) {
      int address = entry.first;
      coverAddressGap(next_address, address);
      next_address = address + entry.second->size;
    }
    coverAddressGap(next_address, end_offset_);
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
          addEntryPoint(parseNumber(tokens[1]), EntryPoint::MANUAL);
        } else {
          cerr << "Syntax: " << cmd << " <address>" << endl; 
        }
      } else if (cmd == "dumpraw") {
        dump_raw_ = true;
      }
    }
  }


  void insertDataFragment(int address) {
    // Find the next address.
    int next_address = end_offset_;
    auto it = disassembly_.upper_bound(address);
    if (it != disassembly_.end()) {
      next_address = it->first;
    }
    ASSERT(next_address > address);
    it--;

    // Find the previous fragment.
    Fragment* prev = it->second.get();
    int prev_address = it->first;
    ASSERT(prev_address < address);

    // Adjust its size.
    prev->size = address - prev_address;

    // Add the new fragment.
    Fragment* data = new Fragment();
    data->type = Fragment::DATA;
    data->size = next_address - address;

    //cerr << "Inserted fragment of size " << (int)data->size << " at "
    //     << Hex16 << address << endl;

    disassembly_[address].reset(data);
  }

  Fragment* getFragment(int address, bool add_if_needed = false) {
    if (disassembly_.count(address) == 0) {
      if (!add_if_needed) {
        return nullptr;
      }
      insertDataFragment(address);
    }
    
    return disassembly_[address].get();
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

        // Attach block comments.
        if (!block_comments.empty()) {
          getFragment(address, true)->block_comments = block_comments;
          block_comments.clear();
        }

        // Find and attach line comment.
        int idx = line.find(';');
        if (idx == string::npos) {
          continue;
        }
        Fragment* fragment = getFragment(address);
        if (fragment) {
          fragment->line_comment = strip(line.substr(idx + 1));
        }
      }
    }

    verifyCoverage();
  }


  void disassembleAndMerge(const string& binary_fn, const string& asm_fn) {
    load(binary_fn);
    disassemble();
    mergeComments(asm_fn);
    outputAsm(asm_fn);
  }


 private:
  map<int, unique_ptr<EntryPoint>> entry_points_;

  Memory* mem_;
  word dummy_;

  bool running_;
  int size_;

  word ip_;
  bool dump_raw_;

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
