#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "helpers.h"

class InterruptHandler {
 public:
  virtual void handleInterrupt(int num) = 0;
};


class IOHandler {
 public:
  virtual byte handleIN(int port) = 0;
  virtual void handleOUT(int port, byte val) = 0;
};

#endif // __DEVICE_H__
