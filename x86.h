#ifndef __X86_H__
#define __X86_H__

#include "x86_base.h"
#include "helpers.h"

#include <unordered_map>

class Memory;
class InterruptHandler;
class IOHandler;

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
  Memory* getMemory();

  virtual void reset();
  virtual void step();

  virtual void registerInterruptHandler(InterruptHandler* handler, int num);
  virtual void registerIOHandler(IOHandler* handler, int num);

 public:

  void setDebugLevel(int level);

  int getCS_IP() const;
  int getSS_SP() const;
 
  int getLinearAddress(word segment, word offset) const;
 
  void check(bool cond, const char* text, const char* file, int line);

  void doPush(word val);
  word doPop();

  virtual byte* getMem8Ptr(word segment, word offset);

  void adjustFlagZS(byte value);
  void adjustFlagZS(word value);

  //
  // X86Base overrides.
  //
  virtual byte fetch() override;
  virtual void notImplemented(const char* opcode_name) override;
  virtual void invalidOpcode() override;

  virtual byte* getReg8Ptr(int reg) override;
  virtual word* getReg16Ptr(int reg) override;

  virtual word* getMem16Ptr(word segment, word offset) override;

  virtual bool getFlag(word mask) const override;
  virtual void setFlag(word mask, bool value) override;


  //
  // Opcode implementations.
  //
  virtual void ADD_w() override;
  virtual void AND_b() override;
  virtual void CALL_w() override;
  virtual void CLD() override;
  virtual void CMP_b() override;
  virtual void CMPSB() override;
  virtual void CMP_w() override;
  virtual void DEC_b() override;
  virtual void INC_w() override;
  virtual void INT() override;
  virtual void JMP_b() override;
  virtual void JMP_w() override;
  virtual void JNZ() override;
  virtual void JZ() override;
  virtual void LOOP() override;
  virtual void LDS() override;
  virtual void MUL_w() override;
  virtual void MOVSB() override;
  virtual void MOVSW() override;
  virtual void MOV_b() override;
  virtual void MOV_w() override;
  virtual void OR_b() override;
  virtual void POP() override;
  virtual void PUSH() override;
  virtual void RET() override;
  virtual void STD() override;
  virtual void SUB_b() override;
  virtual void SUB_w() override;
  virtual void TEST_b() override;
  virtual void XCHG_w() override;
  virtual void XOR_b() override;
  virtual void SHL_wb() override;

 private:
  // Memory and registers.
  Memory* mem_;
  Registers regs_;

  // Interrupt and I/O handlers.
  std::unordered_map<int, InterruptHandler*> int_handlers_;
  std::unordered_map<int, IOHandler*> io_handlers_;

  // Debugging and logging.
  int debug_level_;
};

#endif  // __X86_H__
