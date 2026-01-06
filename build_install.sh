#!/bin/bash
set -e

rm -rf build

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
  -DCMAKE_CXX_FLAGS_MINSIZEREL="-Os -DNDEBUG"

cmake --build build --parallel "$(nproc)"

sudo cmake --install build --strip

echo "Build and installation complete."
