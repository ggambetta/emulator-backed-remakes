// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#include "x86.h"
#include "memory.h"

#include "gtest/gtest.h"

using namespace std;

const int kOffset = 0x100;

class X86Test : public testing::Test {
 public:
  virtual void SetUp() {
    memory_.reset(new Memory(2 << 16));
    x86_.reset(new X86(memory_.get()));
    regs_ = x86_->getRegisters();
    mem_ = memory_->getPointer(0);
  }

 protected:
  unique_ptr<X86> x86_;
  unique_ptr<Memory> memory_;
  Registers* regs_;
  byte* mem_;
};


TEST_F(X86Test, Registers) {
  regs_->ax = 0x1234;
  EXPECT_EQ(0x12, regs_->ah);
  EXPECT_EQ(0x34, regs_->al);
  EXPECT_EQ(0x1234, regs_->regs16[0]);
  EXPECT_EQ(0x34, regs_->regs8[0]);
  EXPECT_EQ(0x12, regs_->regs8[1]);
};


TEST_F(X86Test, SignExtend) {
  EXPECT_EQ(0xFFE8, x86_->signExtend(0xE8));
  EXPECT_EQ(0x0008, x86_->signExtend(0x08));
};

TEST_F(X86Test, LinearAddress) {
  regs_->ss = 0x1234;
  regs_->sp = 0x4567;
  int linear = x86_->getSS_SP();

  EXPECT_EQ(linear, (0x1234 << 4) + 0x4567);
}

TEST_F(X86Test, PUSH_DS) {
  const int stack_top = kOffset + 100;

  EXPECT_EQ(0, mem_[stack_top - 1]);
  EXPECT_EQ(0, mem_[stack_top - 2]);

  regs_->ss = 0;
  regs_->sp = stack_top;
  regs_->ds = 0x1234;
  mem_[kOffset] = 0x1E;  // PUSH DS
  x86_->step();

  EXPECT_EQ(regs_->sp, stack_top - 2);
  EXPECT_EQ(0x34, mem_[stack_top - 2]);
  EXPECT_EQ(0x12, mem_[stack_top - 1]);
}


TEST_F(X86Test, SUB_REG_REG) {
  regs_->ax = 0x1234;

  int off = kOffset;
  mem_[off++] = 0x29;  // SUB AX, AX
  mem_[off++] = 0xC0;

  x86_->step();

  EXPECT_EQ(0, regs_->ax);
}


TEST_F(X86Test, FLAGS) {
  regs_->ax = 0x1234;

  int off = kOffset;
  mem_[off++] = 0x29;  // SUB AX, AX
  mem_[off++] = 0xC0;
  x86_->step();

  EXPECT_EQ(0, regs_->ax);
  EXPECT_TRUE(x86_->getFlag(X86::F_ZF));

  mem_[off++] = 0x40;  // INC AX
  x86_->step();

  EXPECT_EQ(1, regs_->ax);
  EXPECT_FALSE(x86_->getFlag(X86::F_ZF));

  mem_[off++] = 0x00;  // ADD
  mem_[off++] = 0xF0;  // AL, DH
  regs_->al = 0xFF;
  regs_->dh = 0x02;
  x86_->step();

  EXPECT_EQ(0x01, regs_->al);
  EXPECT_TRUE(x86_->getFlag(X86::F_CF));
}


TEST_F(X86Test, PUSH_AX) {
  const int stack_top = kOffset + 100;

  EXPECT_EQ(0, mem_[stack_top - 1]);
  EXPECT_EQ(0, mem_[stack_top - 2]);

  regs_->ss = 0;
  regs_->sp = stack_top;
  regs_->ax = 0x1234;
  mem_[kOffset] = 0x50;  // PUSH AX
  x86_->step();

  EXPECT_EQ(regs_->sp, stack_top - 2);
  EXPECT_EQ(0x34, mem_[stack_top - 2]);
  EXPECT_EQ(0x12, mem_[stack_top - 1]);
}


TEST_F(X86Test, MOV_MEM_REG) {
  regs_->sp = 0x1234;
  regs_->ds = 0;

  int off = kOffset;
  mem_[off++] = 0x89;  // MOV [0x1122], SP
  mem_[off++] = 0x26;
  mem_[off++] = 0x22;
  mem_[off++] = 0x11;

  x86_->step();
  
  EXPECT_EQ(0x34, mem_[0x1122]);
  EXPECT_EQ(0x12, mem_[0x1123]);

  // Now try segment override.
  regs_->sp = 0x5678;
  regs_->es = 0x0100;
  mem_[off++] = 0x26;  // MOV [ES:0x1122], SP
  mem_[off++] = 0x89;
  mem_[off++] = 0x26;
  mem_[off++] = 0x22;
  mem_[off++] = 0x11;

  x86_->step();
  
  EXPECT_EQ(0x34, mem_[0x1122]);  // Unchanged
  EXPECT_EQ(0x12, mem_[0x1123]);

  EXPECT_EQ(0x78, mem_[0x2122]);
  EXPECT_EQ(0x56, mem_[0x2123]);
}


TEST_F(X86Test, XCHG) {
  regs_->si = 0x1234;
  regs_->bx = 0x5678;

  int off = kOffset;
  mem_[off++] = 0x87;  // XCHG SI, BX
  mem_[off++] = 0xf3;

  x86_->step();

  EXPECT_EQ(0x1234, regs_->bx);
  EXPECT_EQ(0x5678, regs_->si);
}


TEST_F(X86Test, CLD) {
  regs_->flags = 0xFFFF;

  int off = kOffset;
  mem_[off++] = 0xFC;  // CLD

  x86_->step();

  EXPECT_EQ(0xFFFF ^ X86::F_DF, regs_->flags);
}


TEST_F(X86Test, MOVSB) {
  regs_->ds = 0x1000;
  regs_->si = 0x0011;

  regs_->es = 0x1100;
  regs_->di = 0x0022;

  int off = kOffset;
  mem_[off++] = 0xFC;  // CLD
  mem_[off++] = 0xA4;  // MOVSB

  mem_[x86_->getLinearAddress(regs_->ds, regs_->si)] = 0x12;
  mem_[x86_->getLinearAddress(regs_->es, regs_->di)] = 0x00;

  x86_->step();
  x86_->step();

  EXPECT_EQ(0x0012, regs_->si);
  EXPECT_EQ(0x0023, regs_->di);
  EXPECT_EQ(0x12, mem_[x86_->getLinearAddress(regs_->es, regs_->di - 1)]);
}


TEST_F(X86Test, REP_MOVSB) {
  regs_->ds = 0x1000;
  regs_->si = 0x0011;

  regs_->es = 0x1100;
  regs_->di = 0x0022;

  regs_->cx = 3;

  int off = kOffset;
  mem_[off++] = 0xFC;  // CLD
  mem_[off++] = 0xF3;  // REP
  mem_[off++] = 0xA4;  // MOVSB

  mem_[x86_->getLinearAddress(regs_->ds, regs_->si + 0)] = 0x11;
  mem_[x86_->getLinearAddress(regs_->ds, regs_->si + 1)] = 0x22;
  mem_[x86_->getLinearAddress(regs_->ds, regs_->si + 2)] = 0x33;

  x86_->step();
  x86_->step();

  EXPECT_EQ(0x0014, regs_->si);
  EXPECT_EQ(0x0025, regs_->di);
  EXPECT_EQ(0x11, mem_[x86_->getLinearAddress(regs_->es, regs_->di - 3)]);
  EXPECT_EQ(0x22, mem_[x86_->getLinearAddress(regs_->es, regs_->di - 2)]);
  EXPECT_EQ(0x33, mem_[x86_->getLinearAddress(regs_->es, regs_->di - 1)]);
  EXPECT_EQ(0, regs_->cx);
}


TEST_F(X86Test, CALL_RET) {
  int off = kOffset;
  mem_[off++] = 0xE8;  // CALL
  mem_[off++] = 0x01;  // 
  mem_[off++] = 0x00;  // $ + 1
  mem_[off++] = 0x90;  // NOP
  mem_[off++] = 0x29;  // SUB AX, AX
  mem_[off++] = 0xC0;
  mem_[off++] = 0xC3;  // RET

  regs_->ax = 0x1234;

  // CALL
  x86_->step();
  EXPECT_EQ(0x0104, regs_->ip);
  EXPECT_EQ(0x1234, regs_->ax);

  // SUB AX, AX
  x86_->step();
  EXPECT_EQ(0x0106, regs_->ip);
  EXPECT_EQ(0, regs_->ax);

  // RET
  x86_->step();
  EXPECT_EQ(0x0103, regs_->ip);

  // NOP
  x86_->step();
}


TEST_F(X86Test, REP_CMPSB) {
  regs_->ds = 0x1000;
  regs_->si = 0x0011;

  regs_->es = 0x1100;
  regs_->di = 0x0022;

  regs_->cx = 3;

  mem_[x86_->getLinearAddress(regs_->ds, regs_->si + 0)] = 0x11;
  mem_[x86_->getLinearAddress(regs_->ds, regs_->si + 1)] = 0x22;
  mem_[x86_->getLinearAddress(regs_->ds, regs_->si + 2)] = 0x33;

  mem_[x86_->getLinearAddress(regs_->es, regs_->di + 0)] = 0x11;
  mem_[x86_->getLinearAddress(regs_->es, regs_->di + 1)] = 0x00;  // <--
  mem_[x86_->getLinearAddress(regs_->es, regs_->di + 2)] = 0x33;

  int off = kOffset;
  mem_[off++] = 0xFC;  // CLD
  mem_[off++] = 0xF3;  // REP
  mem_[off++] = 0xA6;  // CMPSB

  x86_->step();  // CLD
  x86_->step();  // REP CMPSB

  // Stops after 1 even though CX != 0
  EXPECT_EQ(0x0011 + 2, regs_->si);
  EXPECT_EQ(0x0022 + 2, regs_->di);
  EXPECT_EQ(1, regs_->cx);
}


TEST_F(X86Test, MUL) {
  regs_->ax = 3;
  regs_->cx = 5;
  regs_->dx = 0x1234;

  int off = kOffset;
  mem_[off++] = 0xF7;  // MUL
  mem_[off++] = 0xE1;  // CX
  x86_->step();

  EXPECT_EQ(0, regs_->dx);
  EXPECT_EQ(15, regs_->ax);
  EXPECT_EQ(5, regs_->cx);

  EXPECT_FALSE(x86_->getFlag(X86::F_CF));
  EXPECT_FALSE(x86_->getFlag(X86::F_OF));

  // Now multiply big numbers.
  regs_->ax = 0xAA55;
  regs_->cx = 0x1234;
  regs_->dx = 0xFFFF;

  mem_[off++] = 0xF7;  // MUL
  mem_[off++] = 0xE1;  // CX
  x86_->step();

  EXPECT_EQ(0x0C1C, regs_->dx);
  EXPECT_EQ(0x9344, regs_->ax);

  EXPECT_TRUE(x86_->getFlag(X86::F_CF));
  EXPECT_TRUE(x86_->getFlag(X86::F_OF));
}


TEST_F(X86Test, RCL) {
  regs_->bx = 0b0101010100110101;
  x86_->setFlag(X86::F_CF, 1);

  // Word version
  int off = kOffset;
  mem_[off++] = 0xD1;  // RCL BX, 1
  mem_[off++] = 0xD3;
  x86_->step();

  EXPECT_FALSE(x86_->getFlag(X86::F_CF));
  EXPECT_EQ(0b1010101001101011, regs_->bx);

  // Now with carry
  mem_[off++] = 0xD1;  // RCL BX, 1
  mem_[off++] = 0xD3;
  x86_->step();
  
  EXPECT_TRUE(x86_->getFlag(X86::F_CF));
  EXPECT_EQ(0b0101010011010110, regs_->bx);


  // Byte version
  regs_->dl = 0b10011010;
  x86_->setFlag(X86::F_CF, 1);

  mem_[off++] = 0xD0;  // RCL DL, 1
  mem_[off++] = 0xD2;

  x86_->step();
  EXPECT_TRUE(x86_->getFlag(X86::F_CF));
  EXPECT_EQ(0b00110101, regs_->dl);
}
