#!/usr/bin/env bash
# Windows portable .zip builder
#
# Cross-compilation from Linux is NOT available on Lliurex 23
# (mingw-w64 packages are missing from the repositories).
#
# Build natively on Windows using the .bat script:
#   packaging\windows\build-portable.bat

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

echo "MinGW cross-compiler not available on this system."
echo ""
echo "To build for Windows:"
echo "  1. Install Qt 5.15 with MinGW on a Windows machine"
echo "     https://www.qt.io/download-open-source"
echo ""
echo "  2. Open 'Qt 5.15 MinGW' shell from Start menu"
echo ""
echo "  3. Run:"
echo "     cd $PROJECT_ROOT"
echo "     packaging\\windows\\build-portable.bat"
echo ""
echo "The script will build, run windeployqt, and create a .zip."
exit 1
