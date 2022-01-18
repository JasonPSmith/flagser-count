build: flagser-count flagser-count-individ

flagser-count: src/flagser-count.cpp
	@echo "Compiling \"flagser-count\"." && g++ -std=c++14 -O3 -pthread src/flagser-count.cpp -o flagser-count

flagser-count-individ: src/flagser-count-individ.cpp
	@echo "Compiling \"flagser-count-individ\"." && g++ -std=c++14 -O3 -pthread src/flagser-count-individ.cpp -o flagser-count-individ

clean:
	rm -f flagser-count flagser-count-individ
