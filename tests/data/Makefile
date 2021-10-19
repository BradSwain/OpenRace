# If you set LLVM_INSTALL with CMake, these are set to clang 10.0.1 automatically by cmake
# CC=clang
# CXX=clang++
FLAGS=-g -O1 -mllvm -disable-llvm-optzns -fno-discard-value-names -fopenmp -S -emit-llvm

# Check that we are compiling with clang 10.0.1
ifeq ("", "$(shell $(CC) -dumpversion | grep -E '10.0.1')")
$(error "clang 10.0.1 is required to build the test files")
endif
ifeq ("", "$(shell $(CXX) -dumpversion | grep -E '10.0.1')")
$(error "clang 10.0.1 is required to build the test files")
endif

# NOTE: have to add a new '**' for every level of nested subdirectory
SRC=$(wildcard  **/*.c **/**/*.c **/**/**/*.c)
SRC+=$(wildcard  **/*.cpp **/**/*.cpp **/**/**/*.cpp)

OUT=$(patsubst %.c,%.ll, $(patsubst %.cpp,%.ll, ${SRC}))

all: ${OUT}
	
%.ll: %.c
	$(CC) $(FLAGS) $< -o $@

%.ll: %.cpp
	$(CXX) $(FLAGS) $< -o $@

clean:
	@rm -f ${OUT}
