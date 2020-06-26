# flagser-count
A program for counting directed cliques in directed graphs, adapted from https://github.com/luetge/flagser

To install download repository and navigate to flagser-count folder and then run the following commands:
```sh
git clone https://github.com/sparsehash/sparsehash
g++ src/flagser-count.cpp -o flagser-count -std=c++11 -pthread -lz -I./src/
```

To verify that flagser-count has installed correctly run:

```sh
(cd test && python run_test.py)
```
