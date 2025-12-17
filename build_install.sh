#!/bin/bash
set -e

rm -rf build
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
  -DENABLE_INSTALL=ON

cmake --build build --parallel $(nproc)

sudo cmake --install build --strip

echo "Build and installation complete."
