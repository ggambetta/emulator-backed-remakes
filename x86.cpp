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
  current_cs_ = regs_.cs;
  current_ip_ = regs_.ip;
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


void X86::clearFlag(word mask) {
  regs_.flags &= ~mask;
}


void X86::setFlag(word mask) {
  regs_.flags |= mask;
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


void X86::ADD_w() {
  CHECK_WARGS();
  *warg1 += *warg2;
  // TODO: Adjust flags
}


void X86::SUB_w() {
  CHECK_WARGS();
  *warg1 -= *warg2;
  // TODO: Adjust flags
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
  clearFlag(F_DF);
}


void X86::STD() {
  setFlag(F_DF);
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


void X86::XOR_b() {
  CHECK_BARGS();
  *barg1 ^= *barg2;
  
  // TODO: Flags
}


void X86::INT_b() {
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
