CXXFLAGS=-std=c++0x -Wall -g -F /Library/Frameworks -I.. -L../lib 
SDL=-framework SDL2 -framework SDL2_image

BINARIES=goody xgoody

all: library $(BINARIES)

library:
	make -C ../lib
    
clean:
	rm -f *.o $(BINARIES)
	rm -rf *.dSYM

# Binaries.
$(BINARIES): %: %.cpp 
	g++ $(CXXFLAGS) -o $@ $@.cpp -lemu $(SDL)

