#!/bin/bash
set -e

rm -rf build

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

cmake --build build --parallel "$(nproc)"

sudo cmake --install build --strip

echo "Build and installation complete."
