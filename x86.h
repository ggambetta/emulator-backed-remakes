#ifndef __X86_H__
#define __X86_H__

#include "x86_base.h"
#include "helpers.h"

class Memory;

//
// Registers.
//
struct Registers {
  union {
    word regs16[12];
    struct {
      word ax, bx, cx, dx, cs, ds, ss, es, bp, sp, di, si;
    };

    byte regs8[8];
    struct {
      byte al, ah;
      byte bl, bh;
      byte cl, ch;
      byte dl, dh;
    };
  };

  word ip;
};


//
// x86 CPU.
//
class X86 : public X86Base {
 public:
  X86(Memory* mem);

  Registers* getRegisters();

  virtual void reset();
  virtual void step();

 public:

  void setDebugLevel(int level);

  int getCS_IP() const;
  int getSS_SP() const;
 
  int getLinearAddress(word segment, word offset) const;
 
  void incrementAddress(word& segment, word& offset);
  void decrementAddress(word& segment, word& offset);
 
  void check(bool cond, const char* text, const char* file, int line);
 
  //
  // X86Base overrides.
  //
  virtual byte fetch();
  virtual void notImplemented(const char* opcode_name);
  virtual void invalidOpcode();

  virtual word* getReg16Ptr(int reg);
  virtual byte* getReg8Ptr(int reg);

  virtual word* getMem16Ptr(word segment, word offset);

  //
  // Opcode implementations.
  //
  virtual void ADD_w();
  virtual void MOV_w();
  virtual void PUSH();
  virtual void SUB_w();
  virtual void XCHG_w();

 private:
  Memory* mem_;
 
  Registers regs_;
  word current_cs_, current_ip_;

  int debug_level_;
};

#endif  // __X86_H__
