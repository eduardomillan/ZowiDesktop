#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
VERSION=$(grep -oP 'project\(ZowiDesktop\s+VERSION\s+\K\S+(?=\s+LANGUAGES)' "$PROJECT_ROOT/CMakeLists.txt")

echo "=== Generating debian/changelog from CHANGELOG.md ==="
cd "$PROJECT_ROOT"
: "${DISTRO_SUFFIX:=unstable}"
python3 - "$PROJECT_ROOT/CHANGELOG.md" "$PROJECT_ROOT/debian/changelog" "$DISTRO_SUFFIX" "$VERSION" <<'PYEOF'
import sys, re
from datetime import datetime

changelog_md = sys.argv[1]
debian_changelog = sys.argv[2]
distro_suffix = sys.argv[3]
cmake_version = sys.argv[4]

MONTHS = {
    'jan':'Jan','feb':'Feb','mar':'Mar','apr':'Apr','may':'May','jun':'Jun',
    'jul':'Jul','aug':'Aug','sep':'Sep','oct':'Oct','nov':'Nov','dec':'Dec',
    'ene':'Jan','abr':'Apr','ago':'Aug','dic':'Dec',
}

entries = []
version = None
date_str = None
bullets = []
bullet_first = None

def flush_bullet():
    global bullet_first
    if bullet_first is not None:
        bullets.append(bullet_first)
        bullet_first = None

def flush_entry():
    global version, date_str, bullets, bullet_first
    flush_bullet()
    if version:
        entries.append((version, date_str, list(bullets)))
    version = None
    date_str = None
    bullets = []

with open(changelog_md, 'r') as f:
    for line in f:
        m = re.match(r'^## \[([\d.]+)\]\s*-\s*(.+)', line)
        if m:
            flush_entry()
            version = m.group(1)
            date_str = m.group(2).strip()
            continue
        if version is None:
            continue
        if re.match(r'^###\s', line):
            flush_bullet()
            continue
        m = re.match(r'^- (.+)', line)
        if m:
            flush_bullet()
            text = m.group(1).strip()
            text = re.sub(r'\*\*(.+?)\*\*', r'\1', text)
            bullet_first = text
            continue
        m = re.match(r'^  +(\S.+)', line)
        if m and bullet_first is not None and not re.match(r'^  +[-*] ', line):
            text = m.group(1).strip()
            text = re.sub(r'\*\*(.+?)\*\*', r'\1', text)
            bullet_first += ' ' + text
            continue
        if line.strip() == '':
            flush_bullet()
            continue

flush_entry()

def to_rfc2822(ds):
    parts = ds.split()
    now = datetime.now().astimezone()
    tz = now.strftime("%z")
    m = re.match(r'(\d{4})-(\d{2})-(\d{2})', parts[0])
    if m:
        try:
            dt = datetime(int(m.group(1)), int(m.group(2)), int(m.group(3)))
            month = dt.strftime("%b")
            return dt.strftime(f"%a, %d {month} %Y 12:00:00 {tz}")
        except ValueError:
            pass
    if len(parts) >= 3:
        y_s, m_s, d_s = parts[0], parts[1].lower()[:3], parts[2]
        month = MONTHS.get(m_s, m_s.capitalize())
        try:
            dt = datetime.strptime(f"{y_s} {month} {d_s}", "%Y %b %d")
            return dt.strftime(f"%a, %d {month} %Y 12:00:00 {tz}")
        except ValueError:
            pass
    return "Thu, 01 Jan 1970 00:00:00 +0000"

with open(debian_changelog, 'w') as f:
    for i, (ver, ds, bul) in enumerate(entries):
        rfc = to_rfc2822(ds)
        use_version = cmake_version if i == 0 else ver
        f.write(f"zowi-desktop ({use_version}-1+{distro_suffix}) unstable; urgency=medium\n\n")
        if bul:
            for b in bul:
                f.write(f"  * {b}\n")
        else:
            f.write(f"  * Release {use_version}\n")
        f.write(f"\n -- Eduardo Millan <dev@zowi.local>  {rfc}\n\n")
PYEOF

echo "=== Building .deb ==="
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
git checkout -- src/config.json debian/changelog 2>/dev/null || true

echo ""
echo "=== Done ==="
ls -lh "$BUILD_DIR"/zowi-desktop_*.deb 2>/dev/null
