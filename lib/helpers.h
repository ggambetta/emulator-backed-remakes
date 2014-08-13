// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
#ifndef __HELPERS_H__
#define __HELPERS_H__

#include <iostream>
#include <vector>


// Pre-C++11 hacks
#if __cplusplus < 201103L
  #define override
#endif


// Fixed size types.
typedef unsigned char byte;
typedef unsigned short word;


// Logging constants.
extern const char* BINARY2[4];
extern const char* BINARY3[8];
extern const char* REG16_DESC[13];
extern const char* REG8_DESC[8];
extern const char FLAG_NAME[17];


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

void writeBytes(std::ostream& os, byte* data, int count, int width);


// String functions.
std::vector<std::string> split(const std::string& input);

std::string upper(const std::string& str);
std::string lower(const std::string& str);

std::string strip(const std::string& str);


// Returns true if the string contains hex digits or ends with "h".
bool isHexNumber(const std::string& str);

bool parseBool(const std::string& str);
int parseNumber(const std::string& str);


// ASSERT and related functionality.
#define ASSERT(COND) assertHelper(COND, #COND, __FILE__, __LINE__)
#define FATAL(MSG) fatalHelper(MSG, __FILE__, __LINE__)

void fatalHelper(const std::string& msg, const char* file, int line);
void assertHelper(bool cond, const std::string& msg, const char* file, int line);


// Save a RGB buffer to a PPM file.
void saveRGBToPPM(byte* rgb, int width, int height, const std::string& filename);


// Misc.
bool fileExists(const std::string& path);

#endif  // __HELPERS_H__
