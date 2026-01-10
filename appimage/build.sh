#!/usr/bin/bash

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
APPIMAGE_DIR="$ROOT_DIR/appimage"
BUILD_DIR="$APPIMAGE_DIR/build"
APPDIR="$APPIMAGE_DIR/AppDir"

echo "Cleaning"
rm -rf "$BUILD_DIR" "$APPDIR"

mkdir -p "$BUILD_DIR" "$APPDIR"

echo "Configuring"
cd "$BUILD_DIR"
cmake "$ROOT_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr

echo "Building"
make -j"$(nproc)"

echo "Installing to AppDir"
make install DESTDIR="$APPDIR"

echo "Build completed"

echo "Starting appimage-builder."

cd "$APPIMAGE_DIR"
appimage-builder

echo "appimage-builder finished."
