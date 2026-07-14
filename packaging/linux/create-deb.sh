#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== Building .deb ==="
cd "$PROJECT_ROOT"
dpkg-buildpackage -b -us -uc

echo ""
echo "=== Moving artifacts to $BUILD_DIR ==="
mv -f "$PROJECT_ROOT"/../zowi-desktop_*.deb       "$BUILD_DIR/" 2>/dev/null || true
mv -f "$PROJECT_ROOT"/../zowi-desktop-dbgsym_*.ddeb "$BUILD_DIR/" 2>/dev/null || true
mv -f "$PROJECT_ROOT"/../zowi-desktop_*.buildinfo  "$BUILD_DIR/" 2>/dev/null || true
mv -f "$PROJECT_ROOT"/../zowi-desktop_*.changes    "$BUILD_DIR/" 2>/dev/null || true

echo ""
echo "=== Done ==="
ls -lh "$BUILD_DIR"/zowi-desktop_*.deb 2>/dev/null
