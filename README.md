# flagser-count
A program for counting directed cliques in directed graphs, adapted from https://github.com/luetge/flagser

To install download repository with:
```sh
git clone --recursive https://https://github.com/JasonPSmith/flagser-count.git
```
Then install sparsehash with
```sh
(cd sparsehash && ./configure && make && make install && cd ..)
```
Next compile flagser count with
```
make
```

To verify that flagser-count has installed correctly run:

```sh
(cd test && python run_test.py && cd ..)
```

To install pyflagsercount, run:
```sh
pip install .
```
Requirements: numpy and pybind11 are required packages for pyflagsercount
