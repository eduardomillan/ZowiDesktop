#!/usr/bin/env bash
set -e

BUILD_DIR="$(dirname "$0")/build"

cmake -S "$(dirname "$0")" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"

echo ""
echo "Build complete. Run with:"
echo "  $BUILD_DIR/ZowiDesktop"
