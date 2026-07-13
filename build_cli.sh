#!/usr/bin/env bash
set -euo pipefail

QT_PATH="${QT_PATH:-/home/eduardo/Qt/6.5.2/gcc_64}"
BUILD_DIR="build"

echo "=== Building zowi_cli ==="
cmake -B "$BUILD_DIR" \
    -DCMAKE_PREFIX_PATH="$QT_PATH" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DZOWI_BUILD_CLI=ON \
    -DZOWI_BUILD_GUI=OFF \
    -DBUILD_TESTS=OFF

cmake --build "$BUILD_DIR" --target zowi_cli

CLI="./$BUILD_DIR/src/cli/zowi_cli"

echo ""
echo "=== session ==="
echo "$ $CLI session list"
$CLI session list
echo ""

echo "$ $CLI session get wizardDismissed"
$CLI session get wizardDismissed
echo ""

echo "$ $CLI session set wizardDismissed false"
$CLI session set wizardDismissed false
echo ""

echo "$ $CLI session list"
$CLI session list
echo ""

echo "=== config ==="
echo "$ $CLI config list"
$CLI config list
echo ""

echo "$ $CLI config get know_more"
$CLI config get know_more
echo ""

echo "=== translate ==="
echo "$ $CLI translate -s 'Hola mundo'"
$CLI translate -s "Hola mundo"
echo ""

echo "$ $CLI translate -l en_US -s 'Hola mundo'"
$CLI translate -l en_US -s "Hola mundo"
echo ""

echo "=== scan ==="
echo "$ $CLI scan"
$CLI scan
echo ""

echo "=== help ==="
$CLI --help
