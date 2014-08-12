// Emulator-Backed Remakes proof of concept.
// See http://gabrielgambetta.com/remakes.html for background.
//
// (C) 2014 Gabriel Gambetta (gabriel.gambetta@gmail.com)
//
// Licensed under the Whatever/Credit License: you may do whatever you want with
// the code; if you make something cool, credit is appreciated.
//
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
