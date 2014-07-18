#ifndef __VGA_H__
#define __VGA_H__

#include "device.h"

class X86;

class VGA : public InterruptHandler, public IOHandler {
 public:
  VGA(X86* x86);
 
  virtual void handleInterrupt(int num);
  virtual byte handleIN(int port);
  virtual void handleOUT(int port, byte val);

 private:
  X86* x86_;
};

#endif // __VGA_H__
