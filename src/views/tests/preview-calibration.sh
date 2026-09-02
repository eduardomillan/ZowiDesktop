#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# Calibration is only reachable while a robot is connected, so preview it in
# the connected state by default. Pass --step 0..N to open a specific screen:
#   --step 0 = WARNING, --step 1 = LEGS, --step 2 = FEET, --step 3 = CHECK.
exec "$SCRIPT_DIR/preview-screen.sh" CalibrationScreen --connected "$@"