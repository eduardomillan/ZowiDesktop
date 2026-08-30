#!/usr/bin/env bash
set -e

# Publica el repositorio apt firmado (jammy + noble) en la rama gh-pages bajo
# docs/ preservando el website (keep_files). Solo debe ejecutarse de forma
# local/manual, ya que el repo se firma con las claves GPG del mantenedor.
#
# Uso:
#   packaging/publish-apt-repo.sh <VERSION> <BUILD_DIR>
#
# Requisitos:
#   - aptly y gnupg instalados:  sudo apt-get install aptly gnupg
#   - La clave GPG privada importada en tu keyring local
#   - Passphrase disponible via GPG_PASSPHRASE (o APTLY_GPG_PASSPHRASE)

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

REPO_URL=$(gh -C "$PROJECT_ROOT" repo view --json sshUrl -q .sshUrl)
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
echo "  - https://github.com/$(gh -C "$PROJECT_ROOT" repo view --json nameWithOwner -q .nameWithOwner)/tree/gh-pages/docs"
