#include "loader.h"
#include "x86.h"
#include "memory.h"

#include <fstream>
#include <cassert>

using namespace std;

static const int kCOMOffset = 0x0100;

void Loader::loadCOM(const string& filename, X86* x86) {
    Registers* regs = x86->getRegisters();
    Memory* mem = x86->getMemory();

    ifstream file(filename, ios::in | ios::binary | ios::ate);
    int size = (int)file.tellg();
    cout << "File is "<< size << " bytes long." << endl;
    assert(kCOMOffset + size < mem->getSize());

    file.seekg(0, ios::beg);
    file.read((char*)mem->getPointer(kCOMOffset), size);

    regs->cs = regs->ds = regs->es = regs->ss = 0;
    regs->ip = kCOMOffset;

    regs->ss = 0;
    regs->sp = 0xFFFF;
    
    cout << "File successfully loaded." << endl;

}
