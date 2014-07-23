#include "x86.h"

#include "device.h"
#include "memory.h"

#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;

//
// Logging helpers.
//


#define CHECK(COND) check(COND, #COND, __FILE__, __LINE__)

#define CHECK_WARG1() CHECK(warg1 != nullptr);
#define CHECK_WARG2() CHECK(warg2 != nullptr);
#define CHECK_WARGS() CHECK_WARG1(); CHECK_WARG2();

#define CHECK_BARG1() CHECK(barg1 != nullptr);
#define CHECK_BARG2() CHECK(barg2 != nullptr);
#define CHECK_BARGS() CHECK_BARG1(); CHECK_BARG2();

#define CHECK_WBARGS() CHECK_WARG1(); CHECK_BARG2();


//
// x86 CPU.
//
X86::X86(Memory* mem)
  : mem_(mem), debug_level_(0) {
  reset();
}


Registers* X86::getRegisters() {
  return &regs_;
}


Memory* X86::getMemory() {
  return mem_;
}

void X86::reset() {
  X86Base::reset();

  regs_.cs = 0;
  regs_.ip = 0x0100;

  regs_.ss = 0;
  regs_.sp = 0xFFFF;
}


byte X86::fetch() {
  byte val = mem_->read(getCS_IP());
  if (debug_level_ == 2) {
    clog << Addr(current_cs_, current_ip_) << " " << Hex8 << (int)val << endl;
  }
  regs_.ip++;
  return val;
}


int X86::getCS_IP() const {
  return getLinearAddress(regs_.cs, regs_.ip);
}


int X86::getSS_SP() const {
  return getLinearAddress(regs_.ss, regs_.sp);
}


int X86::getLinearAddress(word segment, word offset) const {
  return (segment << 4) + offset;
}


void X86::notImplemented(const char* opcode_name) {
  cerr << "Opcode '" << opcode_name << "' not implemented." << endl;
  cerr << getOpcodeDesc() << endl;
  assert(false);
}

void X86::invalidOpcode() {
  cerr << "Invalid opcode 0x" << Hex8 << (int)opcode_ << endl;
  assert(false);
}

void X86::step() {
  X86Base::step();

  clog << Addr(current_cs_, current_ip_) << " " << getOpcodeDesc() << endl;
}


word* X86::getReg16Ptr(int reg) {
  assert(reg >= 0);
  assert(reg <= R16_COUNT);
  return &regs_.regs16[reg];
}


byte* X86::getReg8Ptr(int reg) {
  assert(reg >= 0);
  assert(reg <= R8_COUNT);
  return &regs_.regs8[reg];
}


word* X86::getMem16Ptr(word segment, word offset) {
  return (word*)mem_->getPointer(getLinearAddress(segment, offset));
}


byte* X86::getMem8Ptr(word segment, word offset) {
  return mem_->getPointer(getLinearAddress(segment, offset));
}


void X86::check(bool cond, const char* text, const char* file, int line) {
  if (cond) {
    return;
  }
  cerr << "Check failed at " << file << ":" << line << ": " << text << endl;
  cerr << Addr(current_cs_, current_ip_) << " "
       << opcode_desc_ << " (0x" << Hex8 << (int)opcode_ << ")" << endl;
  terminate();
}


void X86::setDebugLevel(int debug_level) {
  debug_level_ = debug_level;
}


bool X86::getFlag(word mask) const {
  return (regs_.flags & mask) == mask;
}


void X86::setFlag(word mask, bool value) {
  if (value) {
    regs_.flags |= mask;
  } else {
    regs_.flags &= ~mask;
  }
}


void X86::PUSH() {
  CHECK_WARG1();
  doPush(*warg1);
}


void X86::POP() {
  CHECK_WARG1();
  *warg1 = doPop();
}


word X86::doPop() {
  word val = *(word*)mem_->getPointer(getSS_SP());
  regs_.sp += 2;
  return val;
}


void X86::doPush(word val) {
  //cerr << Addr(regs_.ss, regs_.sp) << endl;
  regs_.sp -= 2;
  *(word*)mem_->getPointer(getSS_SP()) = val;
}


void X86::registerInterruptHandler(InterruptHandler* handler, int num) {
  assert(int_handlers_.count(num) == 0);
  int_handlers_[num] = handler;
}


void X86::registerIOHandler(IOHandler* handler, int num) {
  assert(io_handlers_.count(num) == 0);
  io_handlers_[num] = handler;
}


void X86::adjustFlagZS(byte value) {
  setFlag(F_ZF, value == 0);
  setFlag(F_SF, (value & 0x80) == 0x80);
}


void X86::adjustFlagZS(word value) {
  setFlag(F_ZF, value == 0);
  setFlag(F_SF, (value & 0x8000) == 0x8000);
}


void X86::ADD_w() {
  CHECK_WARGS();
  *warg1 += *warg2;

  // TODO: Adjust flags
  adjustFlagZS(*warg1);
}


void X86::SUB_b() {
  CHECK_BARGS();
  *barg1 -= *barg2;
  // TODO: Adjust flags
  adjustFlagZS(*barg1);
}


void X86::SUB_w() {
  CHECK_WARGS();
  *warg1 -= *warg2;
  // TODO: Adjust flags
  adjustFlagZS(*warg1);
}


void X86::MOV_w() {
  CHECK_WARGS();
  *warg1 = *warg2;
}


void X86::MOV_b() {
  CHECK_BARGS();
  *barg1 = *barg2;
}


void X86::XCHG_w() {
  CHECK_WARGS();
  word tmp = *warg1;
  *warg1 = *warg2;
  *warg2 = tmp;
}


void X86::CLD() {
  setFlag(F_DF, false);
}


void X86::STD() {
  setFlag(F_DF, true);
}


void X86::CMPSB() {
  // Note inverted order of the operands.
  barg2 = getMem8Ptr(regs_.es, regs_.di);
  barg1 = getMem8Ptr(segment_, regs_.si);

  CMP_b();
  
  int inc_dec = (regs_.flags & F_DF) ? -1 : 1;
  regs_.si += inc_dec;
  regs_.di += inc_dec;
}

void X86::MOVSB() {
  barg1 = getMem8Ptr(regs_.es, regs_.di);
  barg2 = getMem8Ptr(segment_, regs_.si);

  *barg1 = *barg2;
  
  int inc_dec = (regs_.flags & F_DF) ? -1 : 1;
  regs_.si += inc_dec;
  regs_.di += inc_dec;
}

void X86::CALL_w() {
  CHECK_WARG1();
  doPush(regs_.ip);
  regs_.ip = *warg1;
}


void X86::JMP_b() {
  CHECK_WARG1();
  regs_.ip = *warg1;
}


void X86::JMP_w() {
  JMP_b();
}


void X86::XOR_b() {
  CHECK_BARGS();
  *barg1 ^= *barg2;

  // TODO: Flags
  adjustFlagZS(*barg1);
  setFlag(F_OF | F_CF, false);
}


void X86::OR_b() {
  CHECK_BARGS();
  *barg1 |= *barg2;

  // TODO: Flags
  adjustFlagZS(*barg1);
  setFlag(F_OF | F_CF, false);
}


void X86::AND_b() {
  CHECK_BARGS();
  *barg1 &= *barg2;

  // TODO: Flags
  adjustFlagZS(*barg1);
  setFlag(F_OF | F_CF, false);
}


void X86::TEST_b() {
  byte val = *barg1;
  AND_b();
  *barg1 = val;
}


void X86::CMP_b() {
  byte val = *barg1;
  SUB_b();
  *barg1 = val;
}


void X86::CMP_w() {
  word val = *warg1;
  SUB_w();
  *warg1 = val;
}


void X86::INT() {
  CHECK_BARG1();
  byte intval = *barg1;
  
  auto handler = int_handlers_.find(intval);
  if (handler != int_handlers_.end()) {
    handler->second->handleInterrupt(intval);
  } else {
    cerr << "No interrupt handler for 0x" << Hex8 << (int)intval << endl;
  }
}


void X86::RET() {
  regs_.ip = doPop(); 
}


void X86::INC_w() {
  CHECK_WARG1();
  *warg1 = (*warg1) + 1;

  // TODO: Flags
  adjustFlagZS(*warg1);
}


void X86::DEC_b() {
  CHECK_BARG1();
  *barg1 = (*barg1) - 1;

  // TODO: Flags
  adjustFlagZS(*barg1);
}
 

void X86::JNZ() {
  CHECK_WARG1();
  if (!getFlag(F_ZF)) {
    regs_.ip = *warg1;
  }
}
 

void X86::JZ() {
  CHECK_WARG1();
  if (getFlag(F_ZF)) {
    regs_.ip = *warg1;
  }
}


void X86::SHL_wb() {
  CHECK_WBARGS();
  *warg1 <<= *barg2;
 
  // TODO: Flags
  adjustFlagZS(*warg1);
}
