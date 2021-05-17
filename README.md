# flagser-count
A program for counting directed cliques in directed graphs, adapted from https://github.com/luetge/flagser

To install download repository with:
```sh
git clone --recursive https://github.com/JasonPSmith/flagser-count.git
```
From the flagser-count directory install sparsehash with
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

If root access is unavailable installation can be done in with the following:
```sh
git clone --recursive https://github.com/JasonPSmith/flagser-count.git
(cd sparsehash && ./configure --prefix=<local_address> && make && make install && cd ..)
find . -type f -name "*.h" -print0 | xargs -0 sed -i 's|<sparsehash\(.*\)>|\"<local_address>/include/sparsehash\1\"|g'
make
pip install . --user
```
