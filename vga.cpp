#include "vga.h"
#include "x86.h"

#include <cassert>
#include <iostream>

using namespace std;


VGA::VGA(X86* x86) : x86_(x86) {
  x86_->registerInterruptHandler(this, 0x10);
}

 
void VGA::handleInterrupt(int num) {
  assert(num == 0x10);
  clog << "VGA handling interrupt" << endl;
}


byte VGA::handleIN(int port) {
  return 0;
}


void VGA::handleOUT(int port, byte val) {

}

