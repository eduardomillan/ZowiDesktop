#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== Building .deb ==="
cd "$PROJECT_ROOT"
# Packaged builds ship with dev mode OFF. It can be re-enabled at runtime
# via the ZOWI_DEV environment variable.
sed -i -E 's/("dev_mode"\s*:\s*")[^"]*(")/\1false\2/' "$PROJECT_ROOT/src/config.json"
mkdir -p "$BUILD_DIR"
dpkg-buildpackage -b -us -uc

echo ""
echo "=== Moving artifacts to $BUILD_DIR ==="
mv -f "$PROJECT_ROOT"/../zowi-desktop_*.deb       "$BUILD_DIR/" 2>/dev/null || true
mv -f "$PROJECT_ROOT"/../zowi-desktop-dbgsym_*.ddeb "$BUILD_DIR/" 2>/dev/null || true
mv -f "$PROJECT_ROOT"/../zowi-desktop_*.buildinfo  "$BUILD_DIR/" 2>/dev/null || true
mv -f "$PROJECT_ROOT"/../zowi-desktop_*.changes    "$BUILD_DIR/" 2>/dev/null || true

# Restore dev mode for the development tree so it is not left OFF after packaging
git checkout -- src/config.json 2>/dev/null || true

echo ""
echo "=== Done ==="
ls -lh "$BUILD_DIR"/zowi-desktop_*.deb 2>/dev/null
