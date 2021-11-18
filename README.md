# flagser-count
A program for counting directed cliques in directed graphs, adapted from https://github.com/luetge/flagser

To install, first download repository with:
```sh
git clone --recursive https://github.com/JasonPSmith/flagser-count.git
```
Next, compile flagser count with
```
make
```

To verify that flagser-count has installed correctly run:

```sh
(cd test && python run_test.py && cd ..)
```

To install pyflagsercount run:
```sh
pip install .
```
Requirements: For pyflagsercount the packages numpy and pybind11 are required, and cmake version ≥ 2.8.12.
