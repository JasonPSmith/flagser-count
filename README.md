# flagser-count
A program for counting directed cliques in directed graphs, adapted from https://github.com/luetge/flagser

To install download repository and navigate to flaser-count folder and then run the following commands:

git clone https://github.com/sparsehash/sparsehash

g++ src/flagser-count.cpp -o flagser-count -std=c++11 -pthread -lz -I./src/

To verrify the flagser-count has installed correctly run:

(cd test && python run_test.py)
