# AGENTS.md

Python 3 bindings (`szn-teng` on PyPI, `python3-teng` in Debian) for the
[Teng](https://github.com/seznam/teng) templating engine (libteng), written
with Boost.Python.

## Layout

- `teng/rawteng.cc` - C++ extension module `rawteng` (Boost.Python, C++17).
- `teng/__init__.py` - pure-Python API (`Teng`, `createDataRoot`,
  `addFragment`, `generatePage`, ...) that wraps `rawteng`.
- `meson.build` - the only build definition (meson-python backend).
- `pyproject.toml` - wheel metadata and cibuildwheel (manylinux) config.
- `subprojects/libteng.wrap` - libteng wrap pinned to a release tag.
- `tools/cibw_before_build_linux.sh` - manylinux prep: installs system deps,
  builds Boost 1.82 with Boost.Python for the current interpreter, writes
  `boost.pc` / `boost-python3.pc`.
- `debian/` - Debian packaging (`dh --buildsystem=meson`).

Note: `rawteng` is installed as a **top-level** module (not `teng.rawteng`);
`teng/__init__.py` does `from rawteng import ...`. Keep it that way.

An untracked `Makefile` may exist locally; ignore it, use Meson.

## Build

Meson defaults to `wrap_mode=nofallback` (system libteng and Boost.Python via
pkg-config, as in Debian). Wheels use `forcefallback` (libteng from the wrap).

Local development build with sanitizers (libteng from the wrap):

```sh
meson setup build -Dbuildtype=debug -Dwrap_mode=forcefallback \
  -Db_sanitize=address,undefined,leak -Db_lundef=false \
  -Dlibp2f:b_sanitize=none -Dcatch2-with-main:b_sanitize=none
meson compile -C build
```

Try it from the build dir (ASan must be preloaded into the interpreter):

```sh
LD_PRELOAD=$(g++ -print-file-name=libasan.so) PYTHONPATH=build:. \
  python3 -c 'import teng; print(teng.Teng("."))'
```

Other targets (do not run unless asked; they need network/containers):

- Wheel: `pip wheel .` (meson-python, `forcefallback`).
- manylinux wheels: `cibuildwheel` (cp311-cp314, x86_64, manylinux_2_28).
- Debian: `dpkg-buildpackage` (needs `libteng-dev`).

Subproject checkouts (`subprojects/*/`, `.wraplock`, generated
`catch2-with-main.wrap`) are gitignored; only hand-written wraps are tracked.

## Tests

There is no test suite in the repository yet. New tests should be pytest
integration tests under `tests/` (run with `pytest`, never
`python -m pytest`) importing the built module as shown above.

## Bumping libteng

Keep the minimum libteng version in sync in three places (see commit
"Lock libteng to v5.0.12"):

- `subprojects/libteng.wrap` - `revision = vX.Y.Z`
- `meson.build` - `dependency('libteng', version: '>= X.Y.Z')`
- `debian/control` - `libteng-dev (>= X.Y.Z)`

Fix libteng bugs upstream in libteng, not by working around them here.

## Commits and releases

- Commit subjects are short imperative sentences; the body explains *why*.
  Subjects and bodies end up verbatim as `debian/changelog` bullets.
- Do not create release commits or tags unless explicitly asked.

## Code style

- C++17, `warning_level=2`; keep the build warning-free.
- Follow the existing style in `rawteng.cc` (4-space indent, Boost.Python
  `bp::` alias, `Name_t` type naming from libteng).
- Python: 3.11+ only, 4-space indent, keep the public API backward compatible
  with the original Seznam teng module.
