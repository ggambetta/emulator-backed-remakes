#include "x86.h"

#include "device.h"
#include "memory.h"

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


const bool X86::byte_parity_[256] = {
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

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

  call_stack_.clear();
}

void X86::refetch() {
  if (!isExecutePending()) {
    return;
  }
  regs_.ip -= getBytesFetched();
  clearExecutionState();
  fetchAndDecode();
}

byte X86::fetch() {
  byte val = mem_->read(getCS_IP());
  regs_.ip++;
  bytes_fetched_++;

  if (debug_level_ >= 2) {
    clog << Addr(current_cs_, current_ip_) << " " << Hex8 << (int)val << endl;
  }
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
  FATAL("Opcode " + string(opcode_name) + " not implemented.");
}

void X86::invalidOpcode() {
  stringstream ss;
  ss << "Invalid opcode 0x" << Hex8 << (int)opcode_;
  FATAL(ss.str());
}


void X86::outputCurrentOperation(std::ostream& os) {
  os << Addr(current_cs_, current_ip_) << " ";

  byte* code = getMem8Ptr(current_cs_, current_ip_);
  writeBytes(os, code, bytes_fetched_, 6);
  os << getOpcodeDesc() << endl;
}


int X86::getBytesFetched() const {
  return bytes_fetched_;
}


void X86::fetchAndDecode() {
  bytes_fetched_ = 0;
  X86Base::fetchAndDecode();

  if (debug_level_ >= 1) {
    outputCurrentOperation(clog);
  }
}


bool X86::isExecutePending() const {
  return (handler_ != nullptr);
}


word* X86::getReg16Ptr(int reg) {
  ASSERT(reg >= 0);
  ASSERT(reg <= R16_COUNT);
  return &regs_.regs16[reg];
}


byte* X86::getReg8Ptr(int reg) {
  ASSERT(reg >= 0);
  ASSERT(reg <= R8_COUNT);
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
  ASSERT(int_handlers_.count(num) == 0);
  int_handlers_[num] = handler;
}


void X86::registerIOHandler(IOHandler* handler, int num) {
  ASSERT(io_handlers_.count(num) == 0);
  io_handlers_[num] = handler;
}


void X86::adjustFlagZSP(byte value) {
  setFlag(F_ZF, value == 0);
  setFlag(F_SF, (value & 0x80) == 0x80);
  setFlag(F_PF, byte_parity_[value]);
}


void X86::adjustFlagZSP(word value) {
  setFlag(F_ZF, value == 0);
  setFlag(F_SF, (value & 0x8000) == 0x8000);
  setFlag(F_PF, byte_parity_[value & 0xFF]);
}


void X86::addEntryPoint() {
  entry_points_.insert(getCS_IP());
}


const vector<pair<word, word>>& X86::getCallStack() const {
  return call_stack_;
}

const unordered_set<int>& X86::getEntryPoints() const {
  return entry_points_;
}

void X86::ADD_w() {
  CHECK_WARGS();

  int result = (*warg1) + (*warg2);
  *warg1 = result & 0xFFFF;

  setFlag(F_CF, (result & 0x10000) != 0);
  adjustFlagZSP(*warg1);
}

void X86::ADC_b() {
  CHECK_BARGS();

  int result = (*barg1) + (*barg2) + getFlag(F_CF);
  *barg1 = result & 0xFF;

  setFlag(F_CF, (result & 0x100) != 0);
  adjustFlagZSP(*barg1);
}

void X86::ADD_b() {
  CHECK_BARGS();

  int result = (*barg1) + (*barg2);
  *barg1 = result & 0xFF;

  setFlag(F_CF, (result & 0x100) != 0);
  adjustFlagZSP(*barg1);
}


void X86::ADD_wb() {
  CHECK_WARG1();
  CHECK_BARG2();

  int result = (*warg1) + (*barg2);
  *warg1 = result & 0xFFFF;

  setFlag(F_CF, (result & 0x10000) != 0);
  adjustFlagZSP(*warg1);
}


void X86::SUB_b() {
  CHECK_BARGS();

  bool borrow = (*barg1) < (*barg2);
  *barg1 -= *barg2;

  setFlag(F_CF, borrow);
  adjustFlagZSP(*barg1);
}


void X86::SUB_w() {
  CHECK_WARGS();

  bool borrow = (*warg1) < (*warg2);
  *warg1 -= *warg2;

  setFlag(F_CF, borrow);
  adjustFlagZSP(*warg1);
}


void X86::SBB_w() {
  CHECK_WARGS();

  bool borrow = (*warg1) < (*warg2) + (int)getFlag(F_CF);
  *warg1 -= *warg2 + (int)getFlag(F_CF);

  setFlag(F_CF, borrow);
  adjustFlagZSP(*warg1);
}


void X86::SBB_b() {
  CHECK_BARGS();

  bool borrow = (*barg1) < (*barg2) + (int)getFlag(F_CF);
  *barg1 -= *barg2 + (int)getFlag(F_CF);

  setFlag(F_CF, borrow);
  adjustFlagZSP(*barg1);
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


void X86::CLI() {
  setFlag(F_IF, false);
}


void X86::STI() {
  setFlag(F_IF, true);
}


void X86::STC() {
  setFlag(F_CF, true);
}

void X86::CLC() {
  setFlag(F_CF, false);
}

void X86::CMC() {
  setFlag(F_CF, !getFlag(F_CF));
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


void X86::MOVSW() {
  warg1 = getMem16Ptr(regs_.es, regs_.di);
  warg2 = getMem16Ptr(segment_, regs_.si);

  *warg1 = *warg2;
  
  int inc_dec = (regs_.flags & F_DF) ? -2 : 2;
  regs_.si += inc_dec;
  regs_.di += inc_dec;
}


void X86::STOSB() {
  barg1 = getMem8Ptr(regs_.es, regs_.di);
  barg2 = &regs_.al;

  *barg1 = *barg2;
  
  int inc_dec = (regs_.flags & F_DF) ? -1 : 1;
  regs_.di += inc_dec;
}


void X86::CALL_w() {
  CHECK_WARG1();
  call_stack_.push_back({regs_.cs, regs_.ip - bytes_fetched_});

  doPush(regs_.ip);
  regs_.ip = *warg1;

  addEntryPoint();
}


void X86::JNB() {
  CHECK_WARG1();
  if (!getFlag(F_CF)) {
    regs_.ip = *warg1;
    addEntryPoint();
  }
}


void X86::JB() {
  CHECK_WARG1();
  if (getFlag(F_CF)) {
    regs_.ip = *warg1;
    addEntryPoint();
  }
}


void X86::JMP_b() {
  CHECK_WARG1();
  regs_.ip = *warg1;
  addEntryPoint();
}


void X86::JMP_w() {
  JMP_b();
}


void X86::XOR_b() {
  CHECK_BARGS();
  *barg1 ^= *barg2;

  adjustFlagZSP(*barg1);
  setFlag(F_OF | F_CF, false);
}


void X86::OR_b() {
  CHECK_BARGS();
  *barg1 |= *barg2;

  adjustFlagZSP(*barg1);
  setFlag(F_OF | F_CF, false);
}


void X86::OR_w() {
  CHECK_WARGS();
  *warg1 |= *warg2;

  adjustFlagZSP(*warg1);
  setFlag(F_OF | F_CF, false);
}


void X86::AND_b() {
  CHECK_BARGS();
  *barg1 &= *barg2;

  adjustFlagZSP(*barg1);
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


void X86::IN_b() {
  CHECK_BARGS();

  auto handler = io_handlers_.find(*barg2);
  if (handler != io_handlers_.end()) {
    *barg1 = handler->second->handleIN(*barg2);
  } else {
    cerr << "No I/O handler for 0x" << Hex8 << (int)(*barg2) << endl;
  }
}


void X86::OUT_b() {
  CHECK_BARGS();

  auto handler = io_handlers_.find(*barg1);
  if (handler != io_handlers_.end()) {
    handler->second->handleOUT(*barg1, *barg2);
  } else {
    cerr << "No I/O handler for 0x" << Hex8 << (int)(*barg1) << endl;
  }
}


void X86::RET() {
  call_stack_.pop_back();
  regs_.ip = doPop(); 
}


void X86::INC_w() {
  CHECK_WARG1();
  *warg1 = (*warg1) + 1;
  adjustFlagZSP(*warg1);
}


void X86::INC_b() {
  CHECK_BARG1();
  *barg1 = (*barg1) + 1;
  adjustFlagZSP(*barg1);
}


void X86::DEC_b() {
  CHECK_BARG1();
  *barg1 = (*barg1) - 1;
  adjustFlagZSP(*barg1);
}

void X86::DEC_w() {
  CHECK_WARG1();
  *warg1 = (*warg1) - 1;
  adjustFlagZSP(*warg1);
}
 

void X86::JNZ() {
  CHECK_WARG1();
  if (!getFlag(F_ZF)) {
    regs_.ip = *warg1;
    addEntryPoint();
  }
}
 

void X86::JZ() {
  CHECK_WARG1();
  if (getFlag(F_ZF)) {
    regs_.ip = *warg1;
    addEntryPoint();
  }
}


void X86::SHL_wb() {
  CHECK_WBARGS();
  *warg1 <<= *barg2;
  adjustFlagZSP(*warg1);
}


void X86::SHL_b() {
  CHECK_BARGS();
  *barg1 <<= *barg2;
  adjustFlagZSP(*barg1);
}

void X86::SHR_b() {
  CHECK_BARGS();
  *barg1 >>= *barg2;
  adjustFlagZSP(*barg1);
}


void X86::MUL_w() {
  CHECK_WARG1();

  unsigned int product = regs_.ax * (*warg1);
  regs_.ax = product & 0xFFFF;
  regs_.dx = product >> 16;

  setFlag(F_CF | F_OF, regs_.dx != 0);
}


void X86::MUL_b() {
  CHECK_BARG1();

  regs_.ax = regs_.al * (*barg1);
  setFlag(F_CF | F_OF, regs_.ah != 0);
}


void X86::LOOP() {
  CHECK_WARG1();

  regs_.cx--;
  if (regs_.cx != 0) {
    regs_.ip = *warg1;
  }
}


void X86::LDS() {
  CHECK_WARGS();

  word* src = getMem16Ptr(regs_.ds, *warg2);
  *warg1 = *src++;
  regs_.ds = *src++;

  cerr << Addr(regs_.ds, *warg1) << endl;
}

void X86::RCL_w() {
  CHECK_WARGS();

  int count = *warg2;
  int value = *warg1;
  while (count--) {
    int cf = (value & 0x8000) != 0;
    value = (value << 1) | getFlag(F_CF);
    setFlag(F_CF, cf);
  }
  *warg1 = value & 0xFFFF;

  if (*warg2 == 1) {
    setFlag(F_OF, ((value & 0x8000) != 0) ^ getFlag(F_CF));
  }
}

void X86::RCL_b() {
  CHECK_BARGS();

  int count = *barg2;
  int value = *barg1;
  while (count--) {
    int cf = (value & 0x80) != 0;
    value = (value << 1) | getFlag(F_CF);
    setFlag(F_CF, cf);
  }
  *barg1 = value & 0xFF;

  if (*barg2 == 1) {
    setFlag(F_OF, ((value & 0x80) != 0) ^ getFlag(F_CF));
  }
}

void X86::RCR_b() {
  CHECK_BARGS();

  int count = *barg2;
  int value = *barg1;

  if (count == 1) {
    setFlag(F_OF, ((value & 0x80) != 0) ^ getFlag(F_CF));
  }

  while (count--) {
    int cf = value & 1;
    value = (value >> 1) | (getFlag(F_CF) << 7);
    setFlag(F_CF, cf);
  }
}
