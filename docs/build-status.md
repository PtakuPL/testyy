# Build status overview

- Builds succeed on Ubuntu 24.04 after installing the dependencies listed in [docs/linux-build-deps.md](./linux-build-deps.md).
- No compiled installer or binaries are stored in the repository; generate them locally with `cmake --build build -j2` after configuring.
- The produced client binary (`./otclient`) is written to the repository root when the build finishes.
- Source file changes related to text rendering (such as `src/framework/graphics/bitmapfont.cpp`) remain present in the repository history; ensure you pull the branch to see them locally.
