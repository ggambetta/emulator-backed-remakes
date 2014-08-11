CXXFLAGS=-std=c++0x -Wall -g -F /Library/Frameworks 
LIBRARY=libemu.a
SDL=-framework SDL2 -framework SDL2_image

BINARIES=runner disassemble goody xgoody
BINARIES_SRC=$(addsuffix .cpp, $(BINARIES))

TEST_SOURCES=$(wildcard *_test.cpp)
TEST_BINARIES=$(basename $(TEST_SOURCES))

GENERATED_FILES=x86_base.cpp x86_base.h

all: library $(BINARIES)

library:
	make -C lib
    
clean:
	make -C lib clean
	rm -f *.o $(BINARIES)
	rm -rf *.dSYM

# Binaries.
$(BINARIES): %: %.cpp 
	g++ $(CXXFLAGS) -o $@ $@.cpp -Llib -lemu $(SDL)

