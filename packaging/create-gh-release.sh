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
PRERELEASE=0
INCLUDE_APPIMAGE=1
INCLUDE_DEB_JAMMY=1
INCLUDE_DEB_NOBLE=1
INCLUDE_WIN_ZIP=1
INCLUDE_WIN_INSTALLER=1

# Workflow front-end (--release / --watch / --dry-run)
RELEASE=0
WATCH=0
DRY_RUN=0

usage() {
    cat <<EOF
Usage: $(basename "$0") [options]

Create a GitHub Release for the current VERSION and attach the artifacts
found in dist/ (Linux AppImage, jammy/noble .deb, Windows portable zip and
installer). Optionally publishes the signed apt repo to gh-pages.

Options:
  --with-apt                  Publish signed apt repo to gh-pages
  --overwrite                 Delete and recreate existing release and tag
  --prerelease                Mark the GitHub Release as a pre-release (not latest)
  --skip-appimage             Do not include Linux AppImage
  --skip-deb-jammy            Do not include Ubuntu 22.04 (Jammy) .deb
  --skip-deb-noble            Do not include Ubuntu 24.04 (Noble) .deb
  --skip-windows-zip          Do not include Windows portable .zip
  --skip-windows-installer    Do not include Windows setup .exe installer

  --release                   Dispatch the 'Release' workflow (release.yml) instead of
                              creating the release from local dist/ artifacts; the CI
                              builds them and runs this script inside the runner
  --watch                     Follow the 'Release' workflow run until it finishes. With
                              --release: the run just dispatched; alone: the most recent
                              run. Prints the failed-step logs if the run fails
  --dry-run                   With --release: print the dispatch command without running
                              the workflow (no CI is launched)
  -h, --help                  Show this help message

Requirements:
  gh CLI installed and authenticated (https://cli.github.com/)
  Local mode: release artifacts present in dist/ (see packaging/linux and the
  Windows CI workflow); --with-apt additionally needs aptly + the GPG key
  Workflow mode (--release/--watch): VERSION and CHANGELOG.md must be committed
  and pushed to the default branch first: CI builds whatever is on that branch

Examples:
  bash packaging/create-gh-release.sh                  # all artifacts (local dist/)
  bash packaging/create-gh-release.sh --with-apt       # also publish apt repo
  bash packaging/create-gh-release.sh --prerelease --skip-windows-installer
  bash packaging/create-gh-release.sh --release        # build + release entirely in CI
  bash packaging/create-gh-release.sh --release --watch   # same, wait for the result
  bash packaging/create-gh-release.sh --watch          # follow the most recent run
  bash packaging/create-gh-release.sh --release --dry-run  # preview the dispatch
EOF
    exit 0
}

for arg in "$@"; do
    case "$arg" in
        --with-apt)
            PUBLISH_APT=1
            ;;
        --overwrite)
            OVERWRITE=1
            ;;
        --prerelease)
            PRERELEASE=1
            ;;
        --release)
            RELEASE=1
            ;;
        --watch)
            WATCH=1
            ;;
        --dry-run)
            DRY_RUN=1
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
            usage
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
    if [ "$PRERELEASE" -eq 1 ]; then
        echo "ERROR: --prerelease cannot be combined with --with-apt: the signed apt repo is the stable channel and must not carry pre-release packages." >&2
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

# ---------------------------------------------------------------------------
# Workflow front-end (--release / --watch)
# ---------------------------------------------------------------------------
# Locate a run of the "Release" workflow and watch it live until it finishes.
#   mode "dispatched" -> find the workflow_dispatch run just created for the
#                        local HEAD (retried, then falls back to the latest run)
#   mode "latest"     -> use the most recent run of the workflow
# Returns 0 on success, 1 on failure (printing the failed-step logs).
watch_release_run() {
    local mode="$1"
    local run_id=""
    if [ "$mode" = "dispatched" ]; then
        local sha
        sha="$(git -C "$PROJECT_ROOT" rev-parse HEAD 2>/dev/null || true)"
        echo ">> Locating the run just dispatched for local HEAD $sha ..."
        local jq_expr=".[] | select(.headSha==\"$sha\") | .databaseId"
        for _ in $(seq 1 10); do
            run_id="$(gh run list -w release.yml --event workflow_dispatch --branch "$DEFAULT_BRANCH" \
                --limit 5 --json databaseId,headSha --jq "$jq_expr" 2>/dev/null | head -1)"
            if [ -n "$run_id" ]; then
                break
            fi
            sleep 3
        done
        if [ -z "$run_id" ]; then
            echo ">> No run with the local HEAD found yet; following the most recent run instead."
        fi
    fi
    if [ -z "$run_id" ]; then
        run_id="$(gh run list -w release.yml --limit 1 --json databaseId --jq '.[0].databaseId' 2>/dev/null)"
    fi
    if [ -z "$run_id" ]; then
        echo "ERROR: no run of the 'Release' workflow (release.yml) found" >&2
        return 1
    fi
    echo ">> Following run $run_id live (gh run watch). Ctrl+C to stop waiting."
    if ! gh run watch "$run_id" --exit-status; then
        echo ">> The 'Release' workflow failed. Logs of the failed step:"
        gh run view "$run_id" --log-failed || true
        echo "ERROR: the 'Release' workflow finished with a failure" >&2
        return 1
    fi
    echo ">> 'Release' workflow completed successfully."
    return 0
}

# Resolve the default branch (origin/HEAD, falling back to main).
DEFAULT_BRANCH="$(git -C "$PROJECT_ROOT" symbolic-ref --quiet refs/remotes/origin/HEAD 2>/dev/null | sed 's|refs/remotes/origin/||' || true)"
DEFAULT_BRANCH="${DEFAULT_BRANCH:-main}"

# --release: dispatch release.yml instead of creating the release from local
# dist/ artifacts. The workflow builds them in CI and then runs this script in
# the runner (local mode), keeping this file the single source of truth.
if [ "$RELEASE" -eq 1 ]; then
    echo "=== Release workflow mode (--release) ==="

    # The workflow checks out the default branch, so VERSION and CHANGELOG.md
    # (the release notes) must already be committed and pushed there.
    git -C "$PROJECT_ROOT" fetch origin "$DEFAULT_BRANCH" >/dev/null 2>&1 || true
    REMOTE_HEAD="$(git -C "$PROJECT_ROOT" rev-parse "origin/$DEFAULT_BRANCH" 2>/dev/null || true)"
    LOCAL_HEAD="$(git -C "$PROJECT_ROOT" rev-parse HEAD)"
    OUT_OF_SYNC=0
    if [ -z "$REMOTE_HEAD" ] || [ "$REMOTE_HEAD" != "$LOCAL_HEAD" ]; then
        OUT_OF_SYNC=1
    fi
    DIRTY_DOCS=0
    if git -C "$PROJECT_ROOT" status --porcelain -- VERSION CHANGELOG.md | grep -q .; then
        DIRTY_DOCS=1
    fi
    if [ "$OUT_OF_SYNC" -eq 1 ] || [ "$DIRTY_DOCS" -eq 1 ]; then
        echo "WARNING: the CI builds from origin/$DEFAULT_BRANCH, so VERSION and"
        echo "         CHANGELOG.md (release notes) must be committed and pushed first."
        if [ "$OUT_OF_SYNC" -eq 1 ]; then
            echo "         Local HEAD differs from origin/$DEFAULT_BRANCH."
        fi
        if [ "$DIRTY_DOCS" -eq 1 ]; then
            echo "         Uncommitted changes to VERSION or CHANGELOG.md."
        fi
        if [ "$DRY_RUN" -eq 0 ]; then
            if [ -t 0 ]; then
                read -rp "Continue dispatching anyway? [y/N] " confirm
                if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
                    echo "Aborted."
                    exit 0
                fi
            else
                echo "ERROR: non-interactive shell and local state is not ready. Push" >&2
                echo "       VERSION + CHANGELOG.md to origin/$DEFAULT_BRANCH first." >&2
                exit 1
            fi
        fi
    fi

    if ! gh workflow view release.yml >/dev/null 2>&1; then
        echo "ERROR: could not find the 'Release' workflow (release.yml) on origin/$DEFAULT_BRANCH." >&2
        exit 1
    fi

    # Map the script flags to the release.yml inputs (all boolean; only inputs
    # that differ from the workflow defaults are passed).
    WF_ARGS=(workflow run release.yml --ref "$DEFAULT_BRANCH")
    [ "$INCLUDE_APPIMAGE" -eq 0 ] && WF_ARGS+=(-F include_appimage=false)
    [ "$INCLUDE_DEB_JAMMY" -eq 0 ] && WF_ARGS+=(-F include_deb_jammy=false)
    [ "$INCLUDE_DEB_NOBLE" -eq 0 ] && WF_ARGS+=(-F include_deb_noble=false)
    [ "$INCLUDE_WIN_ZIP" -eq 0 ] && WF_ARGS+=(-F include_windows_zip=false)
    [ "$INCLUDE_WIN_INSTALLER" -eq 0 ] && WF_ARGS+=(-F include_windows_installer=false)
    [ "$PUBLISH_APT" -eq 1 ] && WF_ARGS+=(-F publish_apt=true)
    [ "$OVERWRITE" -eq 1 ] && WF_ARGS+=(-F overwrite=true)
    [ "$PRERELEASE" -eq 1 ] && WF_ARGS+=(-F prerelease=true)

    echo ""
    echo "Workflow dispatch command:"
    echo "  gh ${WF_ARGS[*]}"
    if [ "$DRY_RUN" -eq 1 ]; then
        echo ""
        echo "=== --dry-run: workflow NOT dispatched. Remove --dry-run to launch it. ==="
        exit 0
    fi

    echo ""
    echo "=== Dispatching the 'Release' workflow (release.yml @ $DEFAULT_BRANCH) ==="
    (cd "$PROJECT_ROOT" && gh "${WF_ARGS[@]}")

    if [ "$WATCH" -eq 1 ]; then
        if ! watch_release_run "dispatched"; then
            exit 1
        fi
    else
        echo ""
        echo "Workflow 'Release' launched. Track it with:"
        echo "  bash packaging/create-gh-release.sh --watch"
        echo "  https://github.com/$(cd "$PROJECT_ROOT" && gh repo view --json nameWithOwner -q .nameWithOwner)/actions/workflows/release.yml"
    fi
    exit 0
fi

# --watch alone: follow the most recent run of the 'Release' workflow.
if [ "$WATCH" -eq 1 ]; then
    echo "=== Watching the most recent 'Release' workflow run ==="
    if ! watch_release_run "latest"; then
        exit 1
    fi
    exit 0
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
CHANGELOG_FILE="$PROJECT_ROOT/CHANGELOG.md"
NOTES=""
if [ -f "$CHANGELOG_FILE" ]; then
    IN_VERSION=0
    while IFS= read -r line; do
        if echo "$line" | grep -qP "^## \\[${VERSION}\\]"; then
            IN_VERSION=1
            continue
        fi
        if [ "$IN_VERSION" -eq 1 ]; then
            if echo "$line" | grep -qP '^## '; then
                break
            fi
            NOTES="${NOTES}${line}"$'\n'
        fi
    done < "$CHANGELOG_FILE"
    NOTES=$(printf '%s' "$NOTES" | sed '/^$/N;/^\n$/D;')
fi
if [ -z "$NOTES" ]; then
    NOTES="Release ${VERSION}"
fi
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
if [ "$PRERELEASE" -eq 1 ]; then
    echo "  (marked as pre-release; GitHub will NOT set it as latest)"
fi
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
GH_RELEASE_ARGS=(release create "$TAG" --title "$TAG" --notes "$NOTES")
if [ "$PRERELEASE" -eq 1 ]; then
    GH_RELEASE_ARGS+=(--prerelease)
fi
GH_RELEASE_ARGS+=("${RELEASE_FILES[@]}")
(cd "$PROJECT_ROOT" && gh "${GH_RELEASE_ARGS[@]}")

REPO="$PROJECT_ROOT"
if [ "$PUBLISH_APT" -eq 1 ]; then
    bash "$PROJECT_ROOT/packaging/publish-apt-repo.sh" "$VERSION" "$DIST_DIR"
fi

echo ""
echo "=== Done ==="
echo "Release: https://github.com/$(cd "$PROJECT_ROOT" && gh repo view --json nameWithOwner -q .nameWithOwner)/releases/tag/$TAG"
