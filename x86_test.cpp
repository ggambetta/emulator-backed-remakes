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


TEST_F(X86Test, LinearAddress) {
  // Increment.
  regs_->ss = 0x1234;
  regs_->sp = 0xFFFF;
  int linear1 = x86_->getSS_SP();

  x86_->incrementAddress(regs_->ss, regs_->sp);
  int linear2 = x86_->getSS_SP();

  EXPECT_EQ(linear2, linear1 + 1);

  // Decrement.
  regs_->ss = 0x1234;
  regs_->sp = 0x0000;
  linear1 = x86_->getSS_SP();

  x86_->decrementAddress(regs_->ss, regs_->sp);
  linear2 = x86_->getSS_SP();

  EXPECT_EQ(linear2, linear1 - 1);
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

  // TODO: Test flags
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
