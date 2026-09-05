#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"
WIN_BUILD_DIR="$PROJECT_ROOT/build-windows"
WIN_DIST_DIR="$WIN_BUILD_DIR/dist"

# Global release flag. When --with-apt is given, the script also builds and
# publishes the signed apt repo (jammy + noble) to the gh-pages branch.
PUBLISH_APT=0
if [ "$1" = "--with-apt" ]; then
    PUBLISH_APT=1
fi

if ! command -v gh &>/dev/null; then
    echo "ERROR: gh CLI is required. Install it from https://cli.github.com/" >&2
    exit 1
fi
if ! gh auth status &>/dev/null; then
    echo "ERROR: gh is not authenticated. Run: gh auth login" >&2
    exit 1
fi

VERSION=$(grep -oP 'project\(ZowiDesktop\s+VERSION\s+\K\S+(?=\s+LANGUAGES)' "$PROJECT_ROOT/CMakeLists.txt")
if [ -z "$VERSION" ]; then
    echo "ERROR: could not read VERSION from CMakeLists.txt" >&2
    exit 1
fi
TAG="v${VERSION}"
echo "Version: $VERSION"
echo "Tag:     $TAG"

echo ""
echo "=== Checking artifacts ==="
MISSING=0
RELEASE_FILES=()

APPIMAGE=$(ls "$DIST_DIR"/ZowiDesktop-*.AppImage 2>/dev/null | head -n1)
if [ -z "$APPIMAGE" ]; then
    echo "  MISSING (dist/): ZowiDesktop-*.AppImage" >&2
    MISSING=1
else
    echo "  OK: $(basename "$APPIMAGE")"
    RELEASE_FILES+=("$APPIMAGE")
fi

DEB_JAMMY=$(ls "$DIST_DIR"/zowi-desktop_"${VERSION}"-1+jammy_amd64.deb 2>/dev/null | head -n1)
if [ -z "$DEB_JAMMY" ]; then
    echo "  MISSING (dist/): zowi-desktop_${VERSION}-1+jammy_amd64.deb" >&2
    MISSING=1
else
    echo "  OK: $(basename "$DEB_JAMMY")"
    RELEASE_FILES+=("$DEB_JAMMY")
fi

DEB_NOBLE=$(ls "$DIST_DIR"/zowi-desktop_"${VERSION}"-1+noble_amd64.deb 2>/dev/null | head -n1)
if [ -z "$DEB_NOBLE" ]; then
    echo "  MISSING (dist/): zowi-desktop_${VERSION}-1+noble_amd64.deb" >&2
    MISSING=1
else
    echo "  OK: $(basename "$DEB_NOBLE")"
    RELEASE_FILES+=("$DEB_NOBLE")
fi

# Windows artifacts (portable zip + installer) are optional but attached when
# present; both live under build-windows/dist/ (see RELEASE.md).
WIN_ZIP=$(ls "$WIN_DIST_DIR"/ZowiDesktop-${VERSION}-windows-x86_64.zip 2>/dev/null | head -n1)
if [ -n "$WIN_ZIP" ]; then
    echo "  OK: $(basename "$WIN_ZIP")"
    RELEASE_FILES+=("$WIN_ZIP")
else
    echo "  (optional) no portable zip found at $WIN_DIST_DIR/ZowiDesktop-${VERSION}-windows-x86_64.zip"
fi

WIN_INSTALLER=$(ls "$WIN_DIST_DIR"/ZowiDesktop-${VERSION}-setup-x64.exe 2>/dev/null | head -n1)
if [ -n "$WIN_INSTALLER" ]; then
    echo "  OK: $(basename "$WIN_INSTALLER")"
    RELEASE_FILES+=("$WIN_INSTALLER")
else
    echo "  (optional) no installer found at $WIN_DIST_DIR/ZowiDesktop-${VERSION}-setup-x64.exe"
fi

if [ "$MISSING" -eq 1 ]; then
    echo ""
    echo "Build the missing Linux artifacts before running this script:" >&2
    echo "  bash packaging/linux/create-appimage.sh" >&2
    echo "  DISTRO_SUFFIX=jammy bash packaging/linux/create-deb.sh   # on Ubuntu 22.04" >&2
    echo "  DISTRO_SUFFIX=noble bash packaging/linux/create-deb.sh   # on Ubuntu 24.04" >&2
    exit 1
fi

echo ""
echo "=== Extracting changelog for $VERSION ==="
CHANGELOG_FILE="$PROJECT_ROOT/debian/changelog"
NOTES=""
if [ -f "$CHANGELOG_FILE" ]; then
    IN_VERSION=0
    while IFS= read -r line; do
        if echo "$line" | grep -qP "^zowi-desktop \(${VERSION}"; then
            IN_VERSION=1
            continue
        fi
        if [ "$IN_VERSION" -eq 1 ]; then
            if echo "$line" | grep -qP '^zowi-desktop \('; then
                break
            fi
            if echo "$line" | grep -qP '^\s+\*'; then
                NOTES="${NOTES}${line}"$'\n'
            fi
        fi
    done < "$CHANGELOG_FILE"
fi
if [ -z "$NOTES" ]; then
    NOTES="Release ${VERSION}"
fi
NOTES=$(echo "$NOTES" | sed 's/^\s*\* /- /' | sed '/^$/d')
echo "$NOTES"

echo ""
echo "=== Checking git status ==="
if git -C "$PROJECT_ROOT" rev-parse "$TAG" &>/dev/null; then
    echo "ERROR: tag $TAG already exists. Delete it first with: git tag -d $TAG" >&2
    exit 1
fi

echo ""
echo "Ready to create tag $TAG and GitHub Release with:"
for f in "${RELEASE_FILES[@]}"; do
    echo "  - $(basename "$f")"
done
if [ "$PUBLISH_APT" -eq 1 ]; then
    echo "  (+ signed apt repo jammy+noble published to gh-pages/docs)"
fi
read -rp "Continue? [y/N] " confirm
if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 0
fi

echo ""
echo "=== Creating tag $TAG ==="
git -C "$PROJECT_ROOT" tag -a "$TAG" -m "Release $VERSION"

echo ""
echo "=== Pushing tag $TAG ==="
git -C "$PROJECT_ROOT" push origin "$TAG"

echo ""
echo "=== Creating GitHub Release ==="
gh -C "$PROJECT_ROOT" release create "$TAG" \
    --title "$TAG" \
    --notes "$NOTES" \
    "${RELEASE_FILES[@]}"

REPO="$PROJECT_ROOT"
if [ "$PUBLISH_APT" -eq 1 ]; then
    bash "$PROJECT_ROOT/packaging/publish-apt-repo.sh" "$VERSION" "$DIST_DIR"
fi

echo ""
echo "=== Done ==="
echo "Release: https://github.com/$(gh -C "$PROJECT_ROOT" repo view --json nameWithOwner -q .nameWithOwner)/releases/tag/$TAG"
