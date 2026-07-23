#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

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
echo "=== Checking artifacts in $BUILD_DIR ==="
MISSING=0

APPIMAGE=$(ls "$BUILD_DIR"/ZowiDesktop-*.AppImage 2>/dev/null | head -n1)
if [ -z "$APPIMAGE" ]; then
    echo "  MISSING: ZowiDesktop-*.AppImage" >&2
    MISSING=1
else
    echo "  OK: $(basename "$APPIMAGE")"
fi

DEB_JAMMY=$(ls "$BUILD_DIR"/zowi-desktop_"${VERSION}"-1+jammy_amd64.deb 2>/dev/null | head -n1)
if [ -z "$DEB_JAMMY" ]; then
    echo "  MISSING: zowi-desktop_${VERSION}-1+jammy_amd64.deb" >&2
    MISSING=1
else
    echo "  OK: $(basename "$DEB_JAMMY")"
fi

DEB_NOBLE=$(ls "$BUILD_DIR"/zowi-desktop_"${VERSION}"-1+noble_amd64.deb 2>/dev/null | head -n1)
if [ -z "$DEB_NOBLE" ]; then
    echo "  MISSING: zowi-desktop_${VERSION}-1+noble_amd64.deb" >&2
    MISSING=1
else
    echo "  OK: $(basename "$DEB_NOBLE")"
fi

if [ "$MISSING" -eq 1 ]; then
    echo ""
    echo "Build the missing artifacts before running this script:" >&2
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
echo "  - $(basename "$APPIMAGE")"
echo "  - $(basename "$DEB_JAMMY")"
echo "  - $(basename "$DEB_NOBLE")"
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
    "$APPIMAGE" \
    "$DEB_JAMMY" \
    "$DEB_NOBLE"

echo ""
echo "=== Done ==="
echo "Release: https://github.com/$(gh -C "$PROJECT_ROOT" repo view --json nameWithOwner -q .nameWithOwner)/releases/tag/$TAG"
