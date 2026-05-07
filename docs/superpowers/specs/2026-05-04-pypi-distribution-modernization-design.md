# PyPI Distribution Modernization (flagser-count → pyflagsercount)

**Date:** 2026-05-04
**Status:** Approved for implementation
**Reference:** Mirror the packaging style of `bigrandomgraphs` (sibling repo at `../bigrandomgraphs`).

## Goal

Convert the Python distribution of `pyflagsercount` from the legacy `setup.py` + custom CMake-driven pybind11 clone to a modern `scikit-build-core` + `pyproject.toml` + `cibuildwheel` pipeline, with GitHub Actions building multi-platform wheels and publishing to TestPyPI / PyPI / signed GitHub releases.

The C++ standalone CLI binaries (`flagser-count`, `flagser-count-individ`) and their `Makefile` are out of scope: they remain in the repo, untouched. Only the Python distribution path is modernized.

## Non-goals

- Refactoring C++ source files (`src/flagser-count.cpp`, `src/flagser-count-individ.cpp`, `include/*.h`).
- Changing the Python API surface in `pyflagsercount/flagser_functions.py`.
- Changing the binding glue in `src/flagser_count_bindings.cpp`.
- Renaming the PyPI project.
- Setting up the PyPI / TestPyPI trusted-publishing accounts (out-of-band; user-controlled).

## Architecture

Two artifacts ship in the wheel:

1. **`pycount`** — pybind11 C++ extension module, installed at the wheel root so `import pycount` works.
2. **`pyflagsercount`** — pure-Python package directory installed alongside, providing the user-facing API. Its existing `flagser_functions.py` already does `from pycount import run_flagser_count`; that contract is preserved.

This mirrors `bigrandomgraphs` exactly: extension `pybrg` at root, Python package `bigrandomgraphs` alongside.

## Files to create

### `pyproject.toml`

```toml
[build-system]
requires = ["scikit-build-core>=0.10", "pybind11>=2.12"]
build-backend = "scikit_build_core.build"

[project]
name = "pyflagsercount"
version = "0.5.0"
description = "A package for counting directed cliques in directed graphs"
readme = "README.md"
license = { file = "LICENSE" }
authors = [{ name = "Jason P. Smith", email = "jasonsmith.bath@gmail.com" }]
requires-python = ">=3.9"
dependencies = ["numpy>=1.17.0"]

[project.urls]
Homepage = "https://github.com/JasonPSmith/flagser-count"

[project.optional-dependencies]
test = ["pytest"]

[tool.pytest.ini_options]
python_files = ["run_test.py", "test_*.py"]

[tool.scikit-build]
minimum-version = "0.10"
cmake.version = ">=3.15"
wheel.packages = ["pyflagsercount"]

[tool.cibuildwheel]
build = ["cp39-*", "cp310-*", "cp311-*", "cp312-*", "cp313-*"]
skip = ["*-musllinux_*", "*_i686"]
test-command = 'python -c "import pyflagsercount; import pycount"'

[tool.cibuildwheel.macos]
archs = ["x86_64", "arm64"]
```

### `.github/workflows/wheels.yml`

Direct adaptation of `bigrandomgraphs/.github/workflows/wheels.yml`. Identical job graph (build_wheels × {ubuntu, macos, windows}; build_sdist; publish-to-testpypi; publish-to-pypi on tag; github-release on tag with sigstore signing). Project URLs change from `bigrandomgraphs` to `pyflagsercount`. The `github-release` job retains the `CHANGELOG.md` extraction step.

### `.gitignore`

Direct copy of `bigrandomgraphs/.gitignore`:

```
build/
dist/
wheelhouse/
*.egg-info/

*.so

__pycache__/
*.py[cod]

.venv/
venv/

.idea/
.vscode/

.DS_Store
```

### `CHANGELOG.md`

Minimal seed required by `wheels.yml`'s `Get Newest Changelog` step. The step reads between two horizontal rules of 79 underscores. Format:

```markdown
# Changelog

_______________________________________________________________________________

## 0.5.0 — 2026-05-04

- Switch build to scikit-build-core + pyproject.toml.
- Add cibuildwheel-based GitHub Actions for Linux/macOS/Windows wheels.
- Add TestPyPI/PyPI trusted-publishing pipeline.
- pybind11 is now a build-time dependency rather than a vendored submodule.

_______________________________________________________________________________

## 0.4.0 and earlier

See git history.
```

## Files to modify

### `CMakeLists.txt`

Replace vendored pybind11 with the pip-installed one, and add an install rule so scikit-build-core can place `pycount` correctly.

```cmake
cmake_minimum_required(VERSION 3.15)
project(pycount LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "6.4.0")
    message(FATAL_ERROR "Insufficient gcc version")
  endif()
endif()

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(pycount src/flagser_count_bindings.cpp)

target_include_directories(pycount PRIVATE .)

if(MSVC)
    target_compile_options(pycount PUBLIC $<$<CONFIG:RELEASE>: /O2>)
    target_compile_options(pycount PUBLIC $<$<CONFIG:DEBUG>:/O1 /DEBUG:FULL /Zi /Zo>)
else()
    target_compile_options(pycount PUBLIC $<$<CONFIG:RELEASE>: -Ofast>)
    target_compile_options(pycount PUBLIC $<$<CONFIG:DEBUG>: -O2 -ggdb -D_GLIBCXX_DEBUG>)
endif()

install(TARGETS pycount DESTINATION .)
```

Notes:
- Bumped `cmake_minimum_required` to 3.15 to match scikit-build-core's CMake floor.
- Dropped MSVC `/Wall` because it's pathological in MSVC (warns on system headers); `/O2` alone matches release intent.

### `README.md`

Add modern install / test sections matching bigrandomgraphs style; keep the existing CLI build instructions (Makefile path) for non-Python users.

## Files to delete

- `setup.py` — replaced by `pyproject.toml`.
- `MANIFEST.in` — replaced by `scikit-build-core`'s `wheel.packages` and CMake-driven file inclusion.
- `.gitmodules` — empty file; pybind11 is now a build-time pip dep.

## Files left untouched

- `Makefile`
- `src/flagser-count.cpp`, `src/flagser-count-individ.cpp`
- `include/*.h`
- `src/flagser_count_bindings.cpp`
- `pyflagsercount/__init__.py`, `pyflagsercount/flagser_functions.py`
- `test/`
- `scripts/`
- `docs/`
- `LICENSE`

## Risks

### Windows portability

The C++ codebase is POSIX-flavored (`-pthread`, `sprintf` with stack-allocated `char` arrays sized via the bit-counting macro). cibuildwheel on `windows-latest` may surface compile errors. Per user decision, Windows stays in the matrix; first CI run is treated as the discovery vehicle. If Windows compilation fails, the fix is out of scope for this design and deferred — the user can drop `windows-latest` from the matrix as a follow-up.

### Trusted publishing is out-of-band

The workflow's `publish-to-testpypi` and `publish-to-pypi` jobs use OIDC trusted publishing. The PyPI / TestPyPI projects must have their trusted-publishing rules registered (repo, workflow filename, environment name) before the workflow can succeed past the build phase. This is a one-time manual setup by whoever controls the PyPI project; the workflow file alone does not configure it.

### `pycount` symbol collision

`pycount` is a generic name and lives at the wheel root, not inside `pyflagsercount`. If another package on PyPI ever ships a top-level `pycount` extension, they will collide in the same environment. Acceptable risk: this name was already used by the existing distribution, so we are preserving an existing contract rather than creating a new one.

### `add_edges` template name in bindings

`flagser_count_bindings.cpp` calls `graph.add_edges(indices_file, indptr_file)` for both uint32 and uint64 instantiations. Whether this resolves correctly across platforms depends on the existing header definitions. Pre-existing behavior; not in scope here.

## Acceptance criteria

1. `pip install .` from a fresh checkout produces a working `pyflagsercount` install on macOS arm64; `python -c "import pyflagsercount; import pycount"` succeeds.
2. `python -m pytest` (or running `test/run_test.py` per the existing convention) still passes against the new build.
3. `pipx run build --sdist` produces an sdist that, when extracted and installed, also builds successfully.
4. The `wheels.yml` workflow passes on Linux and macOS in CI on a non-tag run (Windows status is informational per Risk above).
5. A tag push of the form `vX.Y.Z` triggers PyPI publish + signed GitHub release (verified by the user once trusted publishing is registered).

## Build sequence

1. Create `.gitignore`.
2. Create `CHANGELOG.md`.
3. Replace `CMakeLists.txt`.
4. Create `pyproject.toml`.
5. Delete `setup.py`, `MANIFEST.in`, `.gitmodules`.
6. Update `README.md`.
7. Create `.github/workflows/wheels.yml`.
8. Locally verify `pip install .` and `python -c "import pyflagsercount; import pycount"`.
9. Locally verify `pipx run build --sdist` succeeds.
10. Commit.
