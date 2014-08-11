#include "lib/helpers.h"
#include "lib/vga.h"

#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

const char kFilename[] = "goody.com";

const int kTileOffset = 0x63C2;
const int kTileCount = 256;

const int kFontOffset = 0x73C2;
const int kFontCount = 40;

void dumpImages(byte* data, int width, int height, int count, const string& prefix) {
  byte* rgb = new byte[width*height*3];
  for (int i = 0; i < count; i++) {
    VGA::CGAtoRGB(data, 0, width*height, rgb);
    data += width*height/4;

    stringstream ss;
    ss << prefix << setw(3) << setfill('0') << i << ".ppm";
    saveRGBToPPM(rgb, width, height, ss.str());
  }
  delete [] rgb;

}


int main (int argc, char** argv) {
  ifstream file(kFilename, ios::in | ios::binary | ios::ate);
  int size = (int)file.tellg();

  byte* code = new byte[size + 0x100];
  file.seekg(0, ios::beg);
  file.read((char*)code + 0x100, size);

  // Dump tiles.
  cout << "Saving tile images." << endl;
  dumpImages(code + kTileOffset, 8, 8, kTileCount, "tile_");

  // Dump tiles.
  cout << "Saving font." << endl;
  dumpImages(code + kFontOffset, 8, 8, kFontCount, "text_");

  delete [] code;
  return 0;
}
