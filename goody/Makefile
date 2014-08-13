CXXFLAGS=-std=c++0x -Wall -g -F /Library/Frameworks -I.. -L../lib 

PLATFORM := $(firstword $(shell uname -s))
ifeq ($(PLATFORM),Linux)
	SDL=-lSDL2 -lSDL2_image
else
	SDL=-framework SDL2 -framework SDL2_image
endif

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

