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
    word regs16[13];
    struct {
      word ax, bx, cx, dx, cs, ds, ss, es, bp, sp, di, si, ip;
    };

    byte regs8[8];
    struct {
      byte al, ah;
      byte bl, bh;
      byte cl, ch;
      byte dl, dh;
    };
  };

  word flags;
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
 
  void check(bool cond, const char* text, const char* file, int line);

  void clearFlag(word mask);
  void setFlag(word mask);
 
  void doPush(word val);
  word doPop();

  //
  // X86Base overrides.
  //
  virtual byte fetch();
  virtual void notImplemented(const char* opcode_name);
  virtual void invalidOpcode();

  virtual byte* getReg8Ptr(int reg);
  virtual word* getReg16Ptr(int reg);

  virtual byte* getMem8Ptr(word segment, word offset);
  virtual word* getMem16Ptr(word segment, word offset);

  //
  // Opcode implementations.
  //
  virtual void ADD_w();
  virtual void CALL_w();
  virtual void CLD();
  virtual void INT_b();
  virtual void JMP_b();
  virtual void MOVSB();
  virtual void MOV_b();
  virtual void MOV_w();
  virtual void POP();
  virtual void PUSH();
  virtual void RET();
  virtual void STD();
  virtual void SUB_w();
  virtual void XCHG_w();
  virtual void XOR_b();

 private:
  Memory* mem_;
 
  Registers regs_;
  word current_cs_, current_ip_;

  int debug_level_;
};

#endif  // __X86_H__
