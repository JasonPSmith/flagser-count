
SHELL      := bash
GCC        := g++
openmp_error := $(shell $(GCC) -fopenmp src/flagser.cpp -o ____flagser.o 2>&1 >/dev/null | grep -c "fopenmp")
_          := $(shell rm -f ____flagser.o)
COMPILE    := $(GCC) -std=c++11 -pthread
DEBUG      := -g
DETECTED_OS := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq (0, ${openmp_error})
	COMPILE := ${COMPILE} -fopenmp -D OPENMP_SUPPORT
	OPENMP_MSG := "Found OpenMP support, enabled advanced multithreading."
else
	OPENMP_MSG := "\n\n\x1b[31;01m=========================================================================\n| This compiler does not support OpenMP, to increase performance        |\n| install e.g. g++6. To select the compiler, set the Makefile variable  |\n| GCC appropriately, e.g. \"make GCC=g++-6\"                              |\n=========================================================================\x1b[0m\n\n"
endif

build: flagser-count

flagser-count: src/flagser-count.cpp $(wildcard include/*)
	@echo "Compiling \"flagser-count\"." && ${COMPILE} ${PRODUCTION} src/flagser-count.cpp -o flagser-count -lz -I/home/jason/Documents/packages/sparsehash/src/

clean:
	rm -f flagser
