# PyPI Distribution Modernization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace flagser-count's legacy `setup.py`/vendored-pybind11 build with a `scikit-build-core` + `pyproject.toml` + `cibuildwheel` pipeline matching the sibling `bigrandomgraphs` repo, including a GitHub Actions wheels workflow that publishes to TestPyPI/PyPI.

**Architecture:** Two artifacts ship in the wheel — a pybind11 C++ extension `pycount` installed at the wheel root (so `import pycount` works), and a pure-Python package `pyflagsercount/` installed alongside. The existing C++ source under `src/` and `include/` and the standalone `Makefile` CLI build are untouched. Only the Python distribution is modernized.

**Tech Stack:** scikit-build-core 0.10+, pybind11 2.12+, CMake 3.15+, cibuildwheel 2.22, GitHub Actions, sigstore, PyPI trusted publishing.

**Reference:** Spec at `docs/superpowers/specs/2026-05-04-pypi-distribution-modernization-design.md`. Sibling reference repo at `../bigrandomgraphs/`.

---

## Pre-flight

### Task 0: Verify starting state

**Files:** read-only audit of the working tree.

- [ ] **Step 1: Confirm branch is clean and on `master`**

Run: `git status && git rev-parse --abbrev-ref HEAD`
Expected: working tree clean, branch is `master`. If dirty, stash or commit first.

- [ ] **Step 2: Confirm legacy files exist as the spec describes**

Run: `ls setup.py MANIFEST.in .gitmodules CMakeLists.txt && ls pyflagsercount/ src/ include/`
Expected: all listed files/dirs present.

- [ ] **Step 3: Confirm `pybind11/` is NOT vendored locally**

Run: `ls pybind11 2>&1 || echo "not present"`
Expected: prints `not present`. If a `pybind11/` directory exists, do NOT delete it as part of this plan — flag and stop. (`.gitmodules` is empty per spec, so this should hold.)

---

## Phase 1 — Add new build configuration

### Task 1: Expand `.gitignore`

**Files:**
- Modify: `.gitignore`

The repo already has a one-line `.gitignore` containing `.worktrees/` (added before this branch was cut, to keep the worktree directory out of git). Preserve that line and add the python/build entries on top.

- [ ] **Step 1: Confirm starting content**

Run: `cat .gitignore`
Expected: a single line `.worktrees/`. If you see anything else, stop and report.

- [ ] **Step 2: Overwrite `.gitignore` with the expanded version**

Replace the file with this exact content (note: `.worktrees/` is preserved at the bottom):

```
# Build / packaging outputs
build/
dist/
wheelhouse/
*.egg-info/

# Compiled extension from in-place builds
*.so

# Python bytecode
__pycache__/
*.py[cod]

# Virtual environments
.venv/
venv/

# IDE
.idea/
.vscode/

# macOS
.DS_Store

# Worktrees (subagent-driven development)
.worktrees/
```

- [ ] **Step 3: Verify the file**

Run: `head -1 .gitignore && tail -1 .gitignore`
Expected:

```
# Build / packaging outputs
.worktrees/
```

- [ ] **Step 4: Commit**

```bash
git add .gitignore
git commit -m "build: expand .gitignore for python/build artifacts"
```

---

### Task 2: Create `CHANGELOG.md`

**Files:**
- Create: `CHANGELOG.md`

The release-notes extraction step in `wheels.yml` reads the section between two horizontal rules made of 79 underscores (`_` × 79). The leading rule must come BEFORE the section, and a trailing rule must follow it. We seed two sections so the extractor finds two rules and picks the first.

- [ ] **Step 1: Write `CHANGELOG.md`**

Create `CHANGELOG.md` with this exact content (note: the underscore rules are exactly 79 underscores each):

```markdown
# Changelog

_______________________________________________________________________________

## 0.5.0 — 2026-05-04

- Switch build to scikit-build-core + pyproject.toml.
- Add cibuildwheel-based GitHub Actions for Linux/macOS/Windows wheels.
- Add TestPyPI/PyPI trusted-publishing pipeline and sigstore-signed GitHub releases.
- pybind11 is now a build-time pip dependency rather than a vendored submodule.

_______________________________________________________________________________

## 0.4.0 and earlier

See git history.
```

- [ ] **Step 2: Verify the underscore rules are exactly 79 chars**

Run: `awk '/^_+$/ {print length($0)}' CHANGELOG.md`
Expected: `79` printed twice.

- [ ] **Step 3: Verify the extraction expression from `wheels.yml` produces non-empty output**

Run:

```bash
python -c '
import re
from pathlib import Path
text = re.sub("<!--(.*?)-->", "", Path("CHANGELOG.md").read_text(), flags=re.DOTALL)
start = text.find("_" * 79)
section = text[start:text.find("_" * 79, start + 1)]
print(repr(section[:120]))
assert len(section) > 80, "release-notes extraction produced too-short section"
'
```

Expected: prints a truncated repr of the 0.5.0 release-notes section; the assertion does not fire.

- [ ] **Step 4: Commit**

```bash
git add CHANGELOG.md
git commit -m "docs: add CHANGELOG.md seed for release-notes extraction"
```

---

### Task 3: Replace `CMakeLists.txt`

**Files:**
- Modify: `CMakeLists.txt` (full rewrite)

The existing file uses `add_subdirectory(pybind11)` against a non-existent local clone. Replace with `find_package(pybind11 CONFIG REQUIRED)` (resolves the pip-installed pybind11) and add an `install` rule so scikit-build-core lays the extension at the wheel root.

- [ ] **Step 1: Read the current file to compare**

Run: `cat CMakeLists.txt`
Expected: the existing 27-line file with `add_subdirectory(pybind11)` and no `install()`.

- [ ] **Step 2: Replace `CMakeLists.txt` entirely**

Overwrite with this exact content:

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

Differences from the old file (don't worry — they are intentional):
- `cmake_minimum_required` bumped 3.9 → 3.15 (scikit-build-core's floor).
- Removed `add_subdirectory(pybind11)`; replaced with `find_package(pybind11 CONFIG REQUIRED)`.
- Inlined source path (was `set(BINDINGS_DIR "src")` + variable interpolation).
- Removed MSVC `/Wall` flag (it's pathological in MSVC and warns on system headers — `/O2` alone matches release intent).
- Added `install(TARGETS pycount DESTINATION .)` so scikit-build-core installs the extension at the wheel root.

- [ ] **Step 3: Verify configuration succeeds locally**

Run:

```bash
python -m pip install --quiet --upgrade pip
python -m pip install --quiet pybind11 scikit-build-core cmake
rm -rf /tmp/flagser-count-cmake-check && mkdir /tmp/flagser-count-cmake-check
cd /tmp/flagser-count-cmake-check && cmake -Dpybind11_DIR="$(python -c 'import pybind11; print(pybind11.get_cmake_dir())')" "$OLDPWD" >/tmp/cmake-stdout 2>&1 && cd "$OLDPWD"
tail -5 /tmp/cmake-stdout
```

Expected: ends with `-- Generating done` and `-- Build files have been written to: /tmp/flagser-count-cmake-check`. No `add_subdirectory` or "pybind11 not found" errors.

- [ ] **Step 4: Clean up**

Run: `rm -rf /tmp/flagser-count-cmake-check /tmp/cmake-stdout`

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: switch CMake to find_package(pybind11) and add install rule"
```

---

### Task 4: Create `pyproject.toml`

**Files:**
- Create: `pyproject.toml`

- [ ] **Step 1: Write `pyproject.toml`**

Create `pyproject.toml` with this exact content:

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

- [ ] **Step 2: Verify it parses**

Run: `python -c "import tomllib; tomllib.loads(open('pyproject.toml').read()); print('ok')"`
Expected: `ok`

- [ ] **Step 3: Commit**

```bash
git add pyproject.toml
git commit -m "build: add pyproject.toml with scikit-build-core + cibuildwheel"
```

---

## Phase 2 — Remove legacy build files

### Task 5: Delete `setup.py`, `MANIFEST.in`, `.gitmodules`

**Files:**
- Delete: `setup.py`
- Delete: `MANIFEST.in`
- Delete: `.gitmodules`

- [ ] **Step 1: Confirm nothing else references these files**

Run: `grep -RIn --exclude-dir=.git --exclude-dir=docs --exclude-dir=.idea -E "setup\.py|MANIFEST\.in|gitmodules" .`
Expected: matches only inside `README.md`, `Makefile`, `docs/`, or this plan/spec. If any source code (`src/`, `include/`, `pyflagsercount/`) references them, stop and re-evaluate.

- [ ] **Step 2: Delete the three files**

Run: `git rm setup.py MANIFEST.in .gitmodules`
Expected: all three are removed.

- [ ] **Step 3: Verify they're gone**

Run: `ls setup.py MANIFEST.in .gitmodules 2>&1`
Expected: each line says "No such file or directory".

- [ ] **Step 4: Commit**

```bash
git commit -m "build: remove legacy setup.py, MANIFEST.in, and empty .gitmodules"
```

---

## Phase 3 — Documentation

### Task 6: Update `README.md`

**Files:**
- Modify: `README.md` (full rewrite)

Keep the existing CLI build instructions for non-Python users (Makefile path), and add modern `pip install` + testing sections matching bigrandomgraphs's style.

- [ ] **Step 1: Read current `README.md`**

Run: `cat README.md`
Expected: 28-line file with the existing install instructions.

- [ ] **Step 2: Overwrite `README.md`**

Replace the entire file with this content:

```markdown
# flagser-count

A program for counting directed cliques in directed graphs, adapted from https://github.com/luetge/flagser.

## Python package (pyflagsercount)

```sh
pip install pyflagsercount               # from PyPI (once published)
pip install .                            # from a local checkout
pip install git+https://github.com/JasonPSmith/flagser-count.git
```

Python ≥ 3.9. Source installs need a C++ compiler; CMake ≥ 3.15 is fetched automatically by the build frontend if not already present. Pre-built wheels are produced by CI for Linux x86_64, macOS arm64, macOS x86_64, and Windows x64 across CPython 3.9–3.13.

### Testing

```sh
pip install -e .[test]
(cd test && python run_test.py)
```

The test runner shells out to the standalone `flagser-count` binary, so build it first with `make` (see below) before running.

## Standalone CLI (flagser-count, flagser-count-individ)

To build the C++ command-line binaries, clone with submodules and run `make`:

```sh
git clone --recursive https://github.com/JasonPSmith/flagser-count.git
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
```

- [ ] **Step 3: Verify it parses as Markdown (sanity check)**

Run: `head -1 README.md`
Expected: `# flagser-count`

- [ ] **Step 4: Commit**

```bash
git add README.md
git commit -m "docs: update README with pyproject-based install instructions"
```

---

## Phase 4 — CI workflow

### Task 7: Create `.github/workflows/wheels.yml`

**Files:**
- Create: `.github/workflows/wheels.yml`

This is a near-direct adaptation of `bigrandomgraphs/.github/workflows/wheels.yml`, with the project URLs changed from `bigrandomgraphs` to `pyflagsercount`. Job graph is unchanged: build_wheels × {ubuntu, macos, windows}, build_sdist, publish-to-testpypi (every run), publish-to-pypi (on tag), github-release (on tag, signed by sigstore).

- [ ] **Step 1: Create the directory**

Run: `mkdir -p .github/workflows`
Expected: directory exists; no error.

- [ ] **Step 2: Write `.github/workflows/wheels.yml`**

Create with this exact content:

```yaml
name: Build wheels

on:
  push:
    tags: ["v*"]
  pull_request:
  workflow_dispatch:

jobs:
  build_wheels:
    name: Wheels on ${{ matrix.os }}
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
    steps:
      - uses: actions/checkout@v4

      - name: Build wheels
        uses: pypa/cibuildwheel@v2.22.0

      - uses: actions/upload-artifact@v4
        with:
          name: wheels-${{ matrix.os }}
          path: ./wheelhouse/*.whl

  build_sdist:
    name: Build sdist
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Build sdist
        run: pipx run build --sdist

      - uses: actions/upload-artifact@v4
        with:
          name: sdist
          path: dist/*.tar.gz

  publish-to-testpypi:
    name: Publish Python 🐍 distribution 📦 to TestPyPI
    needs:
      - build_sdist
      - build_wheels
    runs-on: ubuntu-latest

    environment:
      name: testpypi
      url: https://test.pypi.org/p/pyflagsercount

    permissions:
      id-token: write  # IMPORTANT: mandatory for trusted publishing

    steps:
      - name: Download all the dists
        uses: actions/download-artifact@v4
        with:
          merge-multiple: true
          path: dist/
      - name: Publish distribution 📦 to TestPyPI
        uses: pypa/gh-action-pypi-publish@release/v1
        with:
          skip-existing: true
          verbose: true
          repository-url: https://test.pypi.org/legacy/

  publish-to-pypi:
    name: >-
      Publish Python 🐍 distribution 📦 to PyPI
    if: startsWith(github.ref, 'refs/tags/')  # only publish to PyPI on tag pushes
    needs:
      - build_sdist
      - build_wheels
    runs-on: ubuntu-latest
    environment:
      name: pypi
      url: https://pypi.org/p/pyflagsercount
    permissions:
      id-token: write  # IMPORTANT: mandatory for trusted publishing

    steps:
      - name: Download all the dists
        uses: actions/download-artifact@v4
        with:
          merge-multiple: true
          path: dist/
      - name: Publish distribution 📦 to PyPI
        uses: pypa/gh-action-pypi-publish@release/v1
        with:
          skip-existing: true
          verbose: true

  github-release:
    name: >-
      Sign the Python 🐍 distribution 📦 with Sigstore
      and upload them to GitHub Release
    if: startsWith(github.ref, 'refs/tags/')  # only publish to release on tag pushes
    needs:
      - publish-to-pypi
    runs-on: ubuntu-latest

    permissions:
      contents: write  # IMPORTANT: mandatory for making GitHub Releases
      id-token: write  # IMPORTANT: mandatory for sigstore

    steps:
      - name: Checkout Repository
        uses: actions/checkout@v4

      - name: Download Artifacts
        uses: actions/download-artifact@v4
        with:
          merge-multiple: true
          path: dist/

      - name: Sign the dists with Sigstore
        uses: sigstore/gh-action-sigstore-python@v3.0.0
        with:
          inputs: >-
            ./dist/*.whl

      - name: Get Newest Changelog
        run: |
          python -c 'import re; from pathlib import Path; text=re.sub("<!--(.*?)-->", "", (Path.cwd() / "CHANGELOG.md").read_text(), flags=re.DOTALL); print(text); start=text.find("_" * 79); (Path.cwd() / "TEMP_CHANGELOG.md").write_text(text[start:text.find("_" * 79, start+1)])'

      - name: Create GitHub Release
        id: create_release
        uses: softprops/action-gh-release@v1
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
        with:
          tag_name: ${{ github.ref }}
          name: Release ${{ github.ref_name }}
          draft: false
          prerelease: false
          body_path: ./TEMP_CHANGELOG.md
          files: |
            dist/*.*
```

- [ ] **Step 3: Validate YAML syntax**

Run: `python -c "import yaml; yaml.safe_load(open('.github/workflows/wheels.yml'))" && echo ok`
Expected: `ok`. (If `pyyaml` isn't installed, run `python -m pip install --quiet pyyaml` first.)

- [ ] **Step 4: Confirm both PyPI URLs reference `pyflagsercount`, not `bigrandomgraphs`**

Run: `grep -n "p/" .github/workflows/wheels.yml`
Expected:

```
    url: https://test.pypi.org/p/pyflagsercount
    url: https://pypi.org/p/pyflagsercount
```

(No matches containing `bigrandomgraphs`.)

- [ ] **Step 5: Commit**

```bash
git add .github/workflows/wheels.yml
git commit -m "ci: add wheels workflow for cibuildwheel + PyPI trusted publishing"
```

---

## Phase 5 — Local verification

### Task 8: Verify clean local install

**Files:** none — verification only.

- [ ] **Step 1: Create an isolated venv**

Run: `python -m venv /tmp/flagser-verify-venv && /tmp/flagser-verify-venv/bin/python -m pip install --quiet --upgrade pip`
Expected: no errors.

- [ ] **Step 2: Install from current checkout**

Run: `/tmp/flagser-verify-venv/bin/python -m pip install . 2>&1 | tail -20`
Expected: ends with `Successfully installed ... pyflagsercount-0.5.0`. No `pybind11/CMakeLists.txt: No such file` error (which is what would happen if the old `add_subdirectory(pybind11)` line had survived).

- [ ] **Step 3: Verify both modules import**

Run: `/tmp/flagser-verify-venv/bin/python -c "import pyflagsercount; import pycount; print('imports ok')"`
Expected: `imports ok`. If you see `ModuleNotFoundError: No module named 'pycount'`, the `install(TARGETS pycount DESTINATION .)` line in `CMakeLists.txt` is missing or wrong.

- [ ] **Step 4: Run a smoke call against the C++ extension**

Run:

```bash
/tmp/flagser-verify-venv/bin/python -c "
import numpy as np
from pyflagsercount import flagser_count
A = np.array([[0,1,1],[0,0,1],[0,0,0]])
out = flagser_count(A, threads=1)
print(out)
"
```

Expected: prints a dict with at least keys `euler` and `cell_counts`. The exact numbers don't matter — we're checking the binding round-trips.

- [ ] **Step 5: Clean up the venv**

Run: `rm -rf /tmp/flagser-verify-venv`

- [ ] **Step 6: No commit (verification only).**

---

### Task 9: Verify sdist build

**Files:** none — verification only.

- [ ] **Step 1: Build an sdist**

Run: `pipx run build --sdist 2>&1 | tail -5`
Expected: ends with `Successfully built pyflagsercount-0.5.0.tar.gz`.

- [ ] **Step 2: Inspect the sdist contents**

Run: `tar -tzf dist/pyflagsercount-0.5.0.tar.gz | sort`
Expected output includes (at minimum) all of:

```
pyflagsercount-0.5.0/CMakeLists.txt
pyflagsercount-0.5.0/LICENSE
pyflagsercount-0.5.0/PKG-INFO
pyflagsercount-0.5.0/README.md
pyflagsercount-0.5.0/include/argparser.h
pyflagsercount-0.5.0/include/cnpy.h
pyflagsercount-0.5.0/include/definitions.h
pyflagsercount-0.5.0/include/directed_flag_complex.h
pyflagsercount-0.5.0/include/directed_graph.h
pyflagsercount-0.5.0/include/input.h
pyflagsercount-0.5.0/pyflagsercount/__init__.py
pyflagsercount-0.5.0/pyflagsercount/flagser_functions.py
pyflagsercount-0.5.0/pyproject.toml
pyflagsercount-0.5.0/src/flagser_count_bindings.cpp
```

If any `include/` or `src/` file is missing, scikit-build-core's default sdist filter dropped it. Fix by adding `[tool.scikit-build.sdist]` `include = ["include/*", "src/*"]` to `pyproject.toml` and re-run.

- [ ] **Step 3: Install from the sdist into a fresh venv**

Run:

```bash
python -m venv /tmp/flagser-sdist-venv
/tmp/flagser-sdist-venv/bin/python -m pip install --quiet --upgrade pip
/tmp/flagser-sdist-venv/bin/python -m pip install dist/pyflagsercount-0.5.0.tar.gz 2>&1 | tail -5
```

Expected: ends with `Successfully installed ... pyflagsercount-0.5.0`.

- [ ] **Step 4: Verify the sdist install imports correctly**

Run: `/tmp/flagser-sdist-venv/bin/python -c "import pyflagsercount; import pycount; print('sdist imports ok')"`
Expected: `sdist imports ok`.

- [ ] **Step 5: Clean up**

Run: `rm -rf /tmp/flagser-sdist-venv build dist`

- [ ] **Step 6: No commit (verification only).**

---

## Phase 6 — Final checks

### Task 10: Final review and push

**Files:** none — verification + push.

- [ ] **Step 1: Confirm repository state**

Run: `git status && git log --oneline master..HEAD 2>/dev/null || git log --oneline -10`
Expected: working tree clean, 6 new commits on `master` (one per Task 1–7, except Task 5 which is one commit covering 3 deletions).

- [ ] **Step 2: Confirm no stray legacy artifacts**

Run: `ls setup.py MANIFEST.in .gitmodules 2>&1; echo ---; ls .gitignore CHANGELOG.md pyproject.toml .github/workflows/wheels.yml`
Expected:
- First three: each "No such file or directory".
- Last four: each prints the file name with no error.

- [ ] **Step 3: Confirm the CMake file no longer references vendored pybind11**

Run: `grep -n "add_subdirectory" CMakeLists.txt; grep -n "find_package" CMakeLists.txt`
Expected: `add_subdirectory` produces no output. `find_package` line is `find_package(pybind11 CONFIG REQUIRED)`.

- [ ] **Step 4: STOP — do not push**

Per spec acceptance criteria, the user pushes (or merges via PR) themselves. They also need to register PyPI/TestPyPI trusted publishing for the GitHub repo before tagging a release. Hand back to the user with a summary.

---

## Out-of-band setup (NOT part of this plan — user does this themselves)

After this plan is implemented and pushed:

1. Register `pyflagsercount` on TestPyPI with trusted publishing pointing at `JasonPSmith/flagser-count`, workflow file `wheels.yml`, environment `testpypi`.
2. Register `pyflagsercount` on PyPI with trusted publishing pointing at the same repo + workflow, environment `pypi`.
3. Create a GitHub repository environment named `testpypi` (no protection rules needed for first run) and one named `pypi` (recommended: require manual approval before deploys).
4. Push the branch and let the workflow run on `pull_request` to validate the wheel build before tagging.
5. Tag `v0.5.0` to trigger PyPI publish and GitHub release.
