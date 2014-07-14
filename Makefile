CXXFLAGS=-std=c++0x -Wall
TESTFLAGS=-I/usr/local/include -L/usr/local/lib -lgtest -lgtest_main -L. -lemu
LIBRARY=libemu.a

MAIN=main
MAIN_SRC=$(MAIN).cpp

SOURCES=$(filter-out $(MAIN_SRC) %_test.cpp, $(wildcard *.cpp)) generator/x86_base.cpp
OBJECTS=$(addsuffix .o, $(basename $(SOURCES)))

TEST_SOURCES=$(wildcard *_test.cpp)
TEST_BINARIES=$(basename $(TEST_SOURCES))

all: $(MAIN) $(LIBRARY) tests 
    
clean:
	rm -f *.o $(MAIN) $(TEST_BINARIES) run_tests.sh $(LIBRARY)

# Static library.
$(LIBRARY): $(OBJECTS) 
	ar ru $(LIBRARY) $(OBJECTS)

# Main.
$(MAIN): $(MAIN_SRC) $(LIBRARY)
	g++ $(CXXFLAGS) -o $(MAIN) $(MAIN_SRC) -L. -lemu

# Tests.
tests: $(LIBRARY) $(TEST_BINARIES)
	@echo "#!/bin/bash" > run_tests.sh
	@$(foreach BIN, $(TEST_BINARIES), echo ./$(BIN) >> run_tests.sh;)
	@chmod a+x run_tests.sh

$(TEST_BINARIES): %: %.cpp $(LIBRARY)
	g++ $(CXXFLAGS) $(TESTFLAGS) $@.cpp -o $@


