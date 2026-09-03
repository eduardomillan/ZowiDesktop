#!/usr/bin/env bash
set -euo pipefail

# ──────────────────────────────────────────────────────────────────
# verify_arduino_mirrors.sh
#
# Verifies that ZowiDesktop's hand-mirrored protocol constants and
# data catalogs still match the Arduino sources in zowiLibs:
#
#   - Mouth patterns:  arduinolibs/Zowi/{Zowi_mouths.h,Zowi.cpp}
#                      ↔ src/core/src/robot_commands.cpp (kMouthPatterns)
#   - Gestures:        arduinolibs/Zowi/Zowi_gestures.h
#                      ↔ src/core/include/zowi/robot_commands.h (GestureId)
#   - Melody wire order: code/base/ZOWI_BASE_v2.ino receiveSing() switch
#                      ↔ src/core/include/zowi/robot_commands.h (MelodyId)
#   - Command letters: ZOWI_BASE_v2.ino SCmd.addCommand(...)
#                      ↔ src/core/include/zowi/protocol.h (Command enum)
#
# This is the catalog counterpart of sync_firmware_from_zowiLibs.sh
# (the catalogs are mirrors-by-hand, not copies, so they are verified
# rather than overwritten). Exits non-zero on any drift.
#
# Usage:
#   ./scripts/verify_arduino_mirrors.sh [/path/to/zowiLibs]
#
# The path defaults to the ZOWILIBS_PATH environment variable, then
# to /home/eduardo/zowiLibs. Requires python3.
# ──────────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

if [ $# -ge 1 ]; then
    ZOWILIBS_SRC="$1"
elif [ -n "${ZOWILIBS_PATH:-}" ]; then
    ZOWILIBS_SRC="$ZOWILIBS_PATH"
else
    ZOWILIBS_SRC="$HOME/zowiLibs"
fi

if [ ! -d "$ZOWILIBS_SRC/arduinolibs" ]; then
    echo "ERROR: zowiLibs directory not found at: $ZOWILIBS_SRC"
    echo ""
    echo "Clone it first:"
    echo "  git clone https://github.com/eduardomillan/zowiLibs.git"
    echo ""
    echo "Then either pass the path as an argument or set ZOWILIBS_PATH."
    exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "ERROR: python3 is required."
    exit 1
fi

echo "zowiLibs source: $ZOWILIBS_SRC"
echo ""

python3 - "$ZOWILIBS_SRC" "$PROJECT_DIR" <<'PYEOF'
import re
import sys

zowilibs, project = sys.argv[1], sys.argv[2]
errors = 0


def fail(msg):
    global errors
    errors += 1
    print(f"  FAIL  {msg}")


def ok(msg):
    print(f"  ok    {msg}")


def read(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read()


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


# ── 1. Mouth patterns ─────────────────────────────────────────────
print("Mouth patterns (Zowi_mouths.h + Zowi.cpp ↔ kMouthPatterns):")
mouths_h = read(f"{zowilibs}/arduinolibs/Zowi/Zowi_mouths.h")
zowi_cpp = read(f"{zowilibs}/arduinolibs/Zowi/Zowi.cpp")
commands_cpp = read(f"{project}/src/core/src/robot_commands.cpp")

codes = dict(re.findall(r"#define\s+(\w+_code)\s+0b([01]+)", mouths_h))
m = re.search(r"unsigned long int types\s*\[\]\s*=\s*\{([^}]*)\}", strip_comments(zowi_cpp))
if not m:
    fail("could not find getMouthShape() types[] array in Zowi.cpp")
    order = []
else:
    order = [n.strip() for n in m.group(1).split(",") if n.strip()]

arduino_mouths = []
missing = [n for n in order if n not in codes]
if missing:
    fail(f"mouth defines missing in Zowi_mouths.h: {missing}")
arduino_mouths = [codes[n] for n in order if n in codes]

m = re.search(r"constexpr unsigned long kMouthPatterns\[\]\s*=\s*\{(.*?)\};",
              strip_comments(commands_cpp), flags=re.S)
if not m:
    fail("could not find kMouthPatterns[] in robot_commands.cpp")
    desktop_mouths = []
else:
    desktop_mouths = re.findall(r"0b([01]+)", m.group(1))

if len(arduino_mouths) != len(desktop_mouths):
    fail(f"mouth count differs: arduino={len(arduino_mouths)} desktop={len(desktop_mouths)}")
else:
    for i, (a, d) in enumerate(zip(arduino_mouths, desktop_mouths)):
        if a != d:
            fail(f"mouth #{i} pattern differs: arduino=0b{a} desktop=0b{d}")
    if not errors:
        ok(f"{len(desktop_mouths)} patterns identical (zero..angry)")

# ── 2. Gestures ───────────────────────────────────────────────────
print("Gestures (Zowi_gestures.h ↔ GestureId):")
gestures_h = read(f"{zowilibs}/arduinolibs/Zowi/Zowi_gestures.h")
commands_h = read(f"{project}/src/core/include/zowi/robot_commands.h")

pairs = re.findall(r"#define\s+Zowi(\w+)\s+(\d+)", gestures_h)
arduino_gestures = [name for name, _ in sorted(pairs, key=lambda p: int(p[1]))]

m = re.search(r"enum class GestureId\s*:\s*int\s*\{([^}]*)\}", strip_comments(commands_h))
if not m:
    fail("could not find GestureId enum in robot_commands.h")
    desktop_gestures = []
else:
    desktop_gestures = [p.strip().split("=")[0].strip()
                        for p in m.group(1).split(",") if p.strip()]

if len(arduino_gestures) != len(desktop_gestures):
    fail(f"gesture count differs: arduino={len(arduino_gestures)} desktop={len(desktop_gestures)}")
else:
    for i, (a, d) in enumerate(zip(arduino_gestures, desktop_gestures)):
        if a != d:
            fail(f"gesture #{i} differs: arduino=Zowi{a} desktop={d}")
    if not errors:
        ok(f"{len(desktop_gestures)} gestures in the same order (0-based enum = wire id - 1)")

# ── 3. Melody wire order (firmware receiveSing switch) ────────────
print("Melody wire order (ZOWI_BASE_v2.ino receiveSing ↔ MelodyId):")
ino = read(f"{zowilibs}/code/base/ZOWI_BASE_v2.ino")

m = re.search(r"void receiveSing\(\)\s*\{(.*?)\n\}", ino, flags=re.S)
if not m:
    fail("could not find receiveSing() in ZOWI_BASE_v2.ino")
    wire_melodies = []
else:
    cases = re.findall(r"case\s+(\d+)\s*:\s*(?://[^\n]*)?\s*zowi\.sing\((S_\w+)\)",
                       strip_comments(m.group(1)))
    wire_melodies = [s for _, s in sorted(cases, key=lambda c: int(c[0]))]

# Desktop enum name → firmware S_* define (explicit table: the enum mirrors
# the wire order of the switch, not the raw S_* define order).
melody_map = {
    "Connection": "S_connection", "Disconnection": "S_disconnection",
    "Surprise": "S_surprise", "OhOoh": "S_OhOoh", "OhOoh2": "S_OhOoh2",
    "Cuddly": "S_cuddly", "Sleeping": "S_sleeping", "Happy": "S_happy",
    "SuperHappy": "S_superHappy", "HappyShort": "S_happy_short",
    "Sad": "S_sad", "Confused": "S_confused", "Fart1": "S_fart1",
    "Fart2": "S_fart2", "Fart3": "S_fart3", "Mode1": "S_mode1",
    "Mode2": "S_mode2", "Mode3": "S_mode3", "ButtonPushed": "S_buttonPushed",
}

m = re.search(r"enum class MelodyId\s*:\s*int\s*\{([^}]*)\}", strip_comments(commands_h))
if not m:
    fail("could not find MelodyId enum in robot_commands.h")
    desktop_melodies = []
else:
    desktop_melodies = [p.strip().split("=")[0].strip()
                        for p in m.group(1).split(",") if p.strip()]

if len(wire_melodies) != len(desktop_melodies):
    fail(f"melody count differs: firmware wire={len(wire_melodies)} desktop={len(desktop_melodies)}")
else:
    for i, (w, d) in enumerate(zip(wire_melodies, desktop_melodies)):
        expected = melody_map.get(d)
        if expected is None:
            fail(f"desktop melody '{d}' has no entry in verify script's melody_map")
        elif w != expected:
            fail(f"melody K {i + 1} differs: firmware={w} desktop={d} (expected {expected})")
    if not errors:
        ok(f"{len(desktop_melodies)} melodies match the firmware K-switch order")

# ── 4. Command letters ────────────────────────────────────────────
print("Command letters (addCommand in .ino ↔ Command enum):")
m = re.search(r"enum class Command\s*:\s*char\s*\{(.*?)\};", strip_comments(
    read(f"{project}/src/core/include/zowi/protocol.h")), flags=re.S)
if not m:
    fail("could not find Command enum in protocol.h")
    desktop_cmds = set()
else:
    desktop_cmds = set(re.findall(r"=\s*'(.)'", m.group(1)))

ino_cmds = set(re.findall(r'SCmd\.addCommand\(\s*"(.)"', ino))
legacy_cmds = {"N", "U", "B"}  # line-based forms of the old firmware

if not ino_cmds <= desktop_cmds:
    fail(f"firmware commands missing from Command enum: {sorted(ino_cmds - desktop_cmds)}")
if not legacy_cmds <= desktop_cmds:
    fail(f"legacy commands missing from Command enum: {sorted(legacy_cmds - desktop_cmds)}")
if ino_cmds <= desktop_cmds and legacy_cmds <= desktop_cmds:
    ok(f"{len(ino_cmds)} firmware commands + {len(legacy_cmds)} legacy forms covered")

print("")
if errors:
    print(f"DRIFT DETECTED: {errors} error(s). Update the ZowiDesktop mirrors or")
    print("re-run scripts/sync_firmware_from_zowiLibs.sh if the firmware changed.")
    sys.exit(1)
print("All Arduino mirrors verified: mouths, gestures, melodies and commands")
print("match zowiLibs.")
PYEOF
