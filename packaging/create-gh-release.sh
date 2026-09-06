#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"
# Windows artifacts (portable zip + installer) are downloaded/built into the
# same dist/ directory as the Linux artifacts (see RELEASE.md).
WIN_DIST_DIR="$DIST_DIR"

# Release flags and artifact selection
PUBLISH_APT=0
OVERWRITE=0
INCLUDE_APPIMAGE=1
INCLUDE_DEB_JAMMY=1
INCLUDE_DEB_NOBLE=1
INCLUDE_WIN_ZIP=1
INCLUDE_WIN_INSTALLER=1

for arg in "$@"; do
    case "$arg" in
        --with-apt)
            PUBLISH_APT=1
            ;;
        --overwrite)
            OVERWRITE=1
            ;;
        --skip-appimage|--without-appimage)
            INCLUDE_APPIMAGE=0
            ;;
        --skip-deb-jammy|--without-deb-jammy)
            INCLUDE_DEB_JAMMY=0
            ;;
        --skip-deb-noble|--without-deb-noble)
            INCLUDE_DEB_NOBLE=0
            ;;
        --skip-windows-zip|--skip-win-zip|--without-windows-zip|--without-win-zip)
            INCLUDE_WIN_ZIP=0
            ;;
        --skip-windows-installer|--skip-win-installer|--without-windows-installer|--without-win-installer)
            INCLUDE_WIN_INSTALLER=0
            ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --with-apt                  Publish signed apt repo to gh-pages"
            echo "  --overwrite                 Delete and recreate existing release and tag"
            echo "  --skip-appimage             Do not include Linux AppImage"
            echo "  --skip-deb-jammy            Do not include Ubuntu 22.04 (Jammy) .deb"
            echo "  --skip-deb-noble            Do not include Ubuntu 24.04 (Noble) .deb"
            echo "  --skip-windows-zip          Do not include Windows portable .zip"
            echo "  --skip-windows-installer    Do not include Windows setup .exe installer"
            exit 0
            ;;
        *)
            echo "WARNING: unknown option: $arg" >&2
            ;;
    esac
done

if [ "$PUBLISH_APT" -eq 1 ]; then
    if [ "$INCLUDE_DEB_JAMMY" -eq 0 ] || [ "$INCLUDE_DEB_NOBLE" -eq 0 ]; then
        echo "ERROR: --with-apt requires both jammy and noble .deb packages to be included." >&2
        exit 1
    fi
fi

if ! command -v gh &>/dev/null; then
    echo "ERROR: gh CLI is required. Install it from https://cli.github.com/" >&2
    exit 1
fi
if ! gh auth status &>/dev/null; then
    echo "ERROR: gh is not authenticated. Run: gh auth login" >&2
    exit 1
fi

if [ -f "$PROJECT_ROOT/VERSION" ]; then
    VERSION=$(tr -d '\r\n' < "$PROJECT_ROOT/VERSION")
fi
if [ -z "$VERSION" ]; then
    echo "ERROR: could not read VERSION from $PROJECT_ROOT/VERSION" >&2
    exit 1
fi
TAG="v${VERSION}"
echo "Version: $VERSION"
echo "Tag:     $TAG"

echo ""
echo "=== Checking artifacts ==="
MISSING=0
RELEASE_FILES=()

if [ "$INCLUDE_APPIMAGE" -eq 1 ]; then
    APPIMAGE=$(ls "$DIST_DIR"/ZowiDesktop-*.AppImage 2>/dev/null | head -n1)
    if [ -z "$APPIMAGE" ]; then
        echo "  MISSING (dist/): ZowiDesktop-*.AppImage" >&2
        MISSING=1
    else
        echo "  OK: $(basename "$APPIMAGE")"
        RELEASE_FILES+=("$APPIMAGE")
    fi
else
    echo "  SKIPPED: AppImage"
fi

if [ "$INCLUDE_DEB_JAMMY" -eq 1 ]; then
    DEB_JAMMY=$(ls "$DIST_DIR"/zowi-desktop_"${VERSION}"-1+jammy_amd64.deb 2>/dev/null | head -n1)
    if [ -z "$DEB_JAMMY" ]; then
        echo "  MISSING (dist/): zowi-desktop_${VERSION}-1+jammy_amd64.deb" >&2
        MISSING=1
    else
        echo "  OK: $(basename "$DEB_JAMMY")"
        RELEASE_FILES+=("$DEB_JAMMY")
    fi
else
    echo "  SKIPPED: Debian package (Jammy)"
fi

if [ "$INCLUDE_DEB_NOBLE" -eq 1 ]; then
    DEB_NOBLE=$(ls "$DIST_DIR"/zowi-desktop_"${VERSION}"-1+noble_amd64.deb 2>/dev/null | head -n1)
    if [ -z "$DEB_NOBLE" ]; then
        echo "  MISSING (dist/): zowi-desktop_${VERSION}-1+noble_amd64.deb" >&2
        MISSING=1
    else
        echo "  OK: $(basename "$DEB_NOBLE")"
        RELEASE_FILES+=("$DEB_NOBLE")
    fi
else
    echo "  SKIPPED: Debian package (Noble)"
fi

if [ "$INCLUDE_WIN_ZIP" -eq 1 ]; then
    WIN_ZIP=$(ls "$WIN_DIST_DIR"/ZowiDesktop-${VERSION}-windows-x86_64.zip 2>/dev/null | head -n1)
    if [ -n "$WIN_ZIP" ]; then
        echo "  OK: $(basename "$WIN_ZIP")"
        RELEASE_FILES+=("$WIN_ZIP")
    else
        echo "  (optional) no portable zip found at $WIN_DIST_DIR/ZowiDesktop-${VERSION}-windows-x86_64.zip"
    fi
else
    echo "  SKIPPED: Windows portable zip"
fi

if [ "$INCLUDE_WIN_INSTALLER" -eq 1 ]; then
    WIN_INSTALLER=$(ls "$WIN_DIST_DIR"/ZowiDesktop-${VERSION}-setup-x64.exe 2>/dev/null | head -n1)
    if [ -n "$WIN_INSTALLER" ]; then
        echo "  OK: $(basename "$WIN_INSTALLER")"
        RELEASE_FILES+=("$WIN_INSTALLER")
    else
        echo "  (optional) no installer found at $WIN_DIST_DIR/ZowiDesktop-${VERSION}-setup-x64.exe"
    fi
else
    echo "  SKIPPED: Windows installer"
fi

if [ "$MISSING" -eq 1 ]; then
    echo ""
    echo "Build the missing Linux artifacts before running this script:" >&2
    echo "  bash packaging/linux/create-appimage.sh" >&2
    echo "  DISTRO_SUFFIX=jammy bash packaging/linux/create-deb.sh   # on Ubuntu 22.04" >&2
    echo "  DISTRO_SUFFIX=noble bash packaging/linux/create-deb.sh   # on Ubuntu 24.04" >&2
    exit 1
fi

if [ "${#RELEASE_FILES[@]}" -eq 0 ]; then
    echo ""
    echo "ERROR: No release artifacts found or selected to attach to the release." >&2
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
if [ "$OVERWRITE" -eq 1 ]; then
    echo "Notice: --overwrite is set. Cleaning up existing release and tag if present..."
    if (cd "$PROJECT_ROOT" && gh release view "$TAG" &>/dev/null); then
        echo "  Deleting existing GitHub Release $TAG (and its remote tag)..."
        (cd "$PROJECT_ROOT" && gh release delete "$TAG" --yes --cleanup-tag)
    fi
    if git -C "$PROJECT_ROOT" ls-remote --tags origin "$TAG" 2>/dev/null | grep -q "$TAG"; then
        echo "  Deleting remote tag $TAG on origin..."
        git -C "$PROJECT_ROOT" push origin :refs/tags/"$TAG" 2>/dev/null || true
    fi
    if git -C "$PROJECT_ROOT" rev-parse "$TAG" &>/dev/null; then
        echo "  Deleting local tag $TAG..."
        git -C "$PROJECT_ROOT" tag -d "$TAG" 2>/dev/null || true
    fi
fi

TAG_EXISTS=0
if (cd "$PROJECT_ROOT" && gh release view "$TAG" &>/dev/null); then
    echo "ERROR: release $TAG already exists on GitHub. Use --overwrite to replace it." >&2
    exit 1
fi

if git -C "$PROJECT_ROOT" rev-parse "$TAG" &>/dev/null; then
    TAG_COMMIT=$(git -C "$PROJECT_ROOT" rev-parse "${TAG}^{commit}" 2>/dev/null)
    HEAD_COMMIT=$(git -C "$PROJECT_ROOT" rev-parse HEAD)
    if [ "$TAG_COMMIT" = "$HEAD_COMMIT" ]; then
        echo "Notice: tag $TAG already exists and points to HEAD, but release does not exist. Reusing tag."
        TAG_EXISTS=1
    else
        echo "ERROR: tag $TAG already exists and points to $TAG_COMMIT (HEAD is $HEAD_COMMIT)." >&2
        echo "Use --overwrite to replace it, or delete it with: git tag -d $TAG (and git push origin :refs/tags/$TAG)" >&2
        exit 1
    fi
fi

echo ""
if [ "$TAG_EXISTS" -eq 1 ]; then
    echo "Ready to create GitHub Release for existing tag $TAG with:"
else
    echo "Ready to create tag $TAG and GitHub Release with:"
fi
for f in "${RELEASE_FILES[@]}"; do
    echo "  - $(basename "$f")"
done
if [ "$PUBLISH_APT" -eq 1 ]; then
    echo "  (+ signed apt repo jammy+noble published to gh-pages/docs)"
fi
if [ -t 0 ]; then
    read -rp "Continue? [y/N] " confirm
    if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
        echo "Aborted."
        exit 0
    fi
else
    echo "Non-interactive shell detected (e.g. CI); proceeding automatically."
fi

if [ "$TAG_EXISTS" -eq 0 ]; then
    echo ""
    echo "=== Creating tag $TAG ==="
    git -C "$PROJECT_ROOT" tag -a "$TAG" -m "Release $VERSION"

    echo ""
    echo "=== Pushing tag $TAG ==="
    git -C "$PROJECT_ROOT" push origin "$TAG"
else
    echo ""
    echo "=== Reusing existing tag $TAG ==="
    git -C "$PROJECT_ROOT" push origin "$TAG"
fi

echo ""
echo "=== Creating GitHub Release ==="
(cd "$PROJECT_ROOT" && gh release create "$TAG" \
    --title "$TAG" \
    --notes "$NOTES" \
    "${RELEASE_FILES[@]}")

REPO="$PROJECT_ROOT"
if [ "$PUBLISH_APT" -eq 1 ]; then
    bash "$PROJECT_ROOT/packaging/publish-apt-repo.sh" "$VERSION" "$DIST_DIR"
fi

echo ""
echo "=== Done ==="
echo "Release: https://github.com/$(cd "$PROJECT_ROOT" && gh repo view --json nameWithOwner -q .nameWithOwner)/releases/tag/$TAG"
