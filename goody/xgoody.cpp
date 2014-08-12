#include "lib/helpers.h"
#include "lib/vga.h"

#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

const char kFilename[] = "goody.com";
const string kOutDir = "extracted/";

const int kTileOffset = 0x63C2;
const int kTileCount = 256;

const int kFontOffset = 0x73C2;
const int kFontCount = 40;

void dumpImage(byte* data, int width, int height, const string& filename) {
  byte* rgb = new byte[width*height*3];
  VGA::CGAtoRGB(data, 0, width*height, rgb);
  saveRGBToPPM(rgb, width, height, filename);
  delete [] rgb;
}

void dumpImages(byte* data, int width, int height, int count, const string& prefix) {
  for (int i = 0; i < count; i++) {
    stringstream ss;
    ss << prefix << setw(3) << setfill('0') << i << ".ppm";
    dumpImage(data, width, height, ss.str());
    data += width*height/4;
  }
}


int main (int argc, char** argv) {
  ifstream file(kFilename, ios::in | ios::binary | ios::ate);
  int size = (int)file.tellg();

  byte* code = new byte[size + 0x100];
  file.seekg(0, ios::beg);
  file.read((char*)code + 0x100, size);

  // Dump tiles.
  cout << "Saving tile images." << endl;
  dumpImages(code + kTileOffset, 8, 8, kTileCount, kOutDir + "tile_");

  // Dump font.
  cout << "Saving font." << endl;
  dumpImages(code + kFontOffset, 8, 8, kFontCount, kOutDir + "glyph_");

  // Dump images.
  cout << "Saving images" << endl;
  dumpImage(code + 0xE163, 320, 40, kOutDir + "ui.ppm");

  delete [] code;
  return 0;
}
