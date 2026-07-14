#!/usr/bin/env bash
set -euo pipefail

# ──────────────────────────────────────────────────────────────────
# sync_firmware_from_zowiLibs.sh
#
# Copies the pre-compiled firmware HEX files from zowiLibs into
# ZowiDesktop's src/firmware/ directory so the CLI can flash them.
# Also normalises line endings (CRLF → LF) for clean diffs.
#
# Usage:
#   ./scripts/sync_firmware_from_zowiLibs.sh [/path/to/zowiLibs]
#
# The path defaults to the ZOWILIBS_PATH environment variable, then
# to /home/eduardo/zowiLibs.
# ──────────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# ── Locate zowiLibs ───────────────────────────────────────────
if [ $# -ge 1 ]; then
    ZOWILIBS_SRC="$1"
elif [ -n "${ZOWILIBS_PATH:-}" ]; then
    ZOWILIBS_SRC="$ZOWILIBS_PATH"
else
    ZOWILIBS_SRC="$HOME/zowiLibs"
fi

if [ ! -d "$ZOWILIBS_SRC" ]; then
    echo "ERROR: zowiLibs directory not found at: $ZOWILIBS_SRC"
    echo ""
    echo "Clone it first:"
    echo "  git clone https://github.com/eduardomillan/zowiLibs.git"
    echo ""
    echo "Then either pass the path as an argument or set ZOWILIBS_PATH."
    exit 1
fi

echo "zowiLibs source: $ZOWILIBS_SRC"

# ── Copy firmware HEX files ───────────────────────────────────
DEST="$PROJECT_DIR/src/firmware"
mkdir -p "$DEST"

copy_hex() {
    local src="$1"
    local dst="$2"
    if [ -f "$src" ]; then
        cp "$src" "$dst"
        echo "  copied: $(basename "$src")"
        # Normalise CRLF → LF so diffs stay clean
        sed -i 's/\r$//' "$dst" 2>/dev/null || true
        echo "  normalised CRLF → LF"
    else
        echo "  WARNING: source not found: $src"
    fi
}

echo "Copying firmware HEX files..."
copy_hex "$ZOWILIBS_SRC/code .hex/ZOWI_BASE_v2.hex" "$DEST/ZOWI_BASE_v2.hex"

# Alarms and other game variants live in code .ino/games/<name>/.
# If their compiled HEX files exist in zowiLibs they are copied too.
for hex in "$ZOWILIBS_SRC"/code\ .ino/games/*/compiled/*.hex; do
    if [ -f "$hex" ]; then
        copy_hex "$hex" "$DEST/$(basename "$hex")"
    fi
done

# ── Summary ───────────────────────────────────────────────────
echo ""
echo "Done. Files in $DEST:"
ls -1 "$DEST"/*.hex 2>/dev/null || echo "  (no .hex files)"
echo ""
echo "If you just updated zowiLibs, rebuild the CLI and run the core tests:"
echo "  cmake --build build --target zowi_cli -j"
echo "  cmake --build build --target test_robot_commands && ./build/src/core/tests/test_robot_commands"
