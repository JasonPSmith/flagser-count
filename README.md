# flagser-count

A program for counting directed cliques in directed graphs, adapted from https://github.com/luetge/flagser.

## Python package (pyflagsercount)

```sh
pip install pyflagsercount               # from PyPI (once published)
pip install .                            # from a local checkout
pip install git+https://github.com/JasonPSmith/flagser-count.git
```

Python ≥ 3.8. Source installs need a C++ compiler; CMake ≥ 3.15 is fetched automatically by the build frontend if not already present. Pre-built wheels are produced by CI for Linux x86_64, macOS arm64, macOS x86_64, and Windows x64 across CPython 3.9–3.13.

### Testing

```sh
pip install -e .[test]
(cd test && python run_test.py)
```

The test runner shells out to the standalone `flagser-count` binary, so build it first with `make` (see below) before running.

## Standalone CLI (flagser-count, flagser-count-individ)

To build the C++ command-line binaries, clone the repo and run `make`:

```sh
git clone https://github.com/JasonPSmith/flagser-count.git
cd flagser-count
make
```

This produces `./flagser-count` and `./flagser-count-individ` in the repo root. Requirements: a C++14 compiler with `-pthread` support (gcc ≥ 6.4 on Linux).

To verify the CLI:

```sh
(cd test && python run_test.py)
```

## License

See [LICENSE](LICENSE).
