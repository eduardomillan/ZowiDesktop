#!/usr/bin/env bash
set -e

usage() {
    cat <<EOF
Usage: $(basename "$0") <VERSION> <BUILD_DIR>

Publish the signed apt repository (jammy + noble) to the gh-pages branch
under docs/, preserving the website (keep_files). Local/manual only: the
repo is signed with the maintainer's GPG keys.

Arguments:
  VERSION     Version to publish (matches zowi-desktop_<VERSION>-1+jammy/noble .deb)
  BUILD_DIR   Directory containing the jammy and noble .deb files (e.g. dist/)

Environment:
  GPG_PASSPHRASE           Passphrase to unlock the signing key
  APTLY_GPG_PASSPHRASE     Fallback for the same passphrase

Requirements:
  aptly and gnupg installed: sudo apt-get install aptly gnupg
  Private GPG key imported in the local keyring
  Passphrase available via GPG_PASSPHRASE (or APTLY_GPG_PASSPHRASE)

Example:
  GPG_PASSPHRASE=secret bash packaging/publish-apt-repo.sh 1.2.3 dist
EOF
    exit 0
}

for arg in "$@"; do
    case "$arg" in
        -h|--help|--usage) usage ;;
    esac
done

# Publishes the signed apt repo (jammy + noble) to gh-pages/docs preserving the
# website. Manual/local only: the repo is signed with the maintainer's GPG keys.

set -o pipefail

VERSION="${1:?usage: publish-apt-repo.sh <VERSION> <BUILD_DIR>}"
BUILD_DIR="${2:?usage: publish-apt-repo.sh <VERSION> <BUILD_DIR>}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if ! command -v aptly &>/dev/null; then
    echo "ERROR: aptly is required. Install it: sudo apt-get install aptly" >&2
    exit 1
fi
if ! command -v gpg &>/dev/null; then
    echo "ERROR: gnupg is required. Install it: sudo apt-get install gnupg" >&2
    exit 1
fi

PASSPHRASE="${GPG_PASSPHRASE:-${APTLY_GPG_PASSPHRASE:-}}"
if [ -z "$PASSPHRASE" ]; then
    echo "ERROR: set GPG_PASSPHRASE (or APTLY_GPG_PASSPHRASE) to unlock the signing key." >&2
    exit 1
fi

DEB_JAMMY=$(ls "$BUILD_DIR"/zowi-desktop_"${VERSION}"-1+jammy_amd64.deb 2>/dev/null | head -n1)
DEB_NOBLE=$(ls "$BUILD_DIR"/zowi-desktop_"${VERSION}"-1+noble_amd64.deb 2>/dev/null | head -n1)
if [ -z "$DEB_JAMMY" ] || [ -z "$DEB_NOBLE" ]; then
    echo "ERROR: both jammy and noble .deb are required in $BUILD_DIR" >&2
    exit 1
fi

export APTLY_GPG_PASSPHRASE="$PASSPHRASE"

echo "=== Building signed apt repo (jammy + noble) ==="
aptly repo drop zowi-jammy >/dev/null 2>&1 || true
aptly repo drop zowi-noble >/dev/null 2>&1 || true
aptly publish drop jammy >/dev/null 2>&1 || true
aptly publish drop noble >/dev/null 2>&1 || true

aptly repo create -distribution=jammy -component=main zowi-jammy
aptly repo add zowi-jammy "$DEB_JAMMY"
aptly publish repo -distribution=jammy zowi-jammy

aptly repo create -distribution=noble -component=main zowi-noble
aptly repo add zowi-noble "$DEB_NOBLE"
aptly publish repo -distribution=noble zowi-noble

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

echo ""
echo "=== Staging apt repo under docs/ (keep_files keeps the website) ==="
mkdir -p "$WORK/publish/docs"
cp -r ~/.aptly/public/dists "$WORK/publish/docs/"
cp -r ~/.aptly/public/pool "$WORK/publish/docs/"
gpg --export > "$WORK/publish/docs/keyring.gpg"
touch "$WORK/publish/.nojekyll"

REPO_URL=$(cd "$PROJECT_ROOT" && gh repo view --json sshUrl -q .sshUrl)
echo "=== Publishing to gh-pages ($REPO_URL) ==="
rm -rf "$WORK/ghpages"
git clone -b gh-pages --single-branch "$REPO_URL" "$WORK/ghpages"

# keep_files: copia-los nuevos/actualizados bajo docs/ sin borrar nada existente.
mkdir -p "$WORK/ghpages/docs"
cp -r "$WORK/publish/docs/." "$WORK/ghpages/docs/"

cd "$WORK/ghpages"
if [ -n "$(git status --porcelain)" ]; then
    git add docs/
    git -c user.name="$(git config user.name || echo zowi-release)" \
        -c user.email="$(git config user.email || echo zowi-release@localhost)" \
        commit -m "publish apt repo: v${VERSION} (jammy+noble)"
    git push origin gh-pages
else
    echo "Nothing to commit (apt repo unchanged for v${VERSION})."
fi

echo ""
echo "=== Apt repo published ==="
echo "  - https://github.com/$(cd "$PROJECT_ROOT" && gh repo view --json nameWithOwner -q .nameWithOwner)/tree/gh-pages/docs"
