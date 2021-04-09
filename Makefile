build: flagser-count

flagser-count: src/flagser-count.cpp
	@echo "Compiling \"flagser-count\"." && g++ -std=c++11 -pthread src/flagser-count.cpp -o flagser-count -lz -I./sparsehash/src

clean:
	rm -f flagser-count
