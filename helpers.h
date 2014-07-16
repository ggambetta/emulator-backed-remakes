#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <iostream>

// Fixed size types.
typedef unsigned char byte;
typedef unsigned short word;

// Logging constants.
extern const char* BINARY2[4];
extern const char* BINARY3[8];
extern const char* REG16_DESC[12];
extern const char* REG8_DESC[8];

// Logging helpers.
struct _Hex8 {};
std::ostream& operator<< (std::ostream& os, const _Hex8&);
extern _Hex8 Hex8;

struct _Hex16 {};
std::ostream& operator<< (std::ostream& os, const _Hex16&);
extern _Hex16 Hex16;

struct Addr {
  Addr(word seg, word off) : seg_(seg), off_(off) {}
  word seg_, off_;
};
std::ostream& operator<< (std::ostream& os, const Addr& addr); 


std::string hex8ToString(byte val);
std::string hex16ToString(word val);
std::string addrToString(const Addr& addr);

#endif  // __HELPERS_H__
