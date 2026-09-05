# Releasing Zowi Desktop

Releases are **manual by design**: there is no automated release workflow. A
release bundles artifacts built on two platforms — **Linux** (AppImage + Debian
packages) and **Windows** (portable zip + Inno Setup installer) — and uploads
them to a GitHub Release, optionally publishing a signed apt repository.

This document is the end-to-end guide. For low-level build details (toolchains,
`bt_native`, QML deployment) see [docs/project/BUILD.md](BUILD.md).

## Artifacts

| Platform | Artifact | Produced by |
|---|---|---|
| Linux | `dist/ZowiDesktop-<version>-x86_64.AppImage` | `packaging/linux/create-appimage.sh` |
| Linux | `dist/zowi-desktop_<version>-1+jammy_amd64.deb` | `packaging/linux/create-deb.sh` (Ubuntu 22.04) |
| Linux | `dist/zowi-desktop_<version>-1+noble_amd64.deb` | `packaging/linux/create-deb.sh` (Ubuntu 24.04) |
| Windows | `dist/ZowiDesktop-<version>-windows-x86_64.zip` | CI (`windows.yml`) or local `build-portable.bat` |
| Windows | `dist/ZowiDesktop-<version>-setup-x64.exe` | CI (`windows.yml`) or local `build-installer.bat` |

The AppImage and both `.deb` files are **mandatory** for a release; the Windows
artifacts are attached automatically when present.

## Release version

The version lives in a single source of truth, the top of `CMakeLists.txt`:

```cmake
project(ZowiDesktop VERSION 0.6.0 LANGUAGES CXX)
```

All release scripts parse the `project()` line with a regex (`VERSION\s+(\S+)`)
and derive the git tag as `v<version>`. To cut a new release:

1. Bump the version in `CMakeLists.txt`.
2. Add the matching entry at the top of `CHANGELOG.md`:

   ```markdown
   ## [0.7.0] - 2026-09-15

   ### Added
   - Short bullet describing the feature

   ### Changed
   - Short bullet describing the change

   ### Fixed
   - Short bullet describing the fix
   ```

   Bullets that start with `- ` become the GitHub Release notes (see below).

## Release checklist

1. Bump `CMakeLists.txt` and update `CHANGELOG.md`.
2. Build the **Linux** artifacts (AppImage + jammy/noble `.deb`).
3. Build or download the **Windows** artifacts (zip + installer).
4. `gh auth login` and commit the regenerated `debian/changelog`.
5. Run `packaging/create-gh-release.sh` (or with `--with-apt`).

## Linux artifacts

### Prerequisites

- A Linux machine with the build dependencies listed in
  [docs/project/BUILD.md](BUILD.md) (CMake, Qt 6, compiler).
- `wget` (used by the AppImage script to download `linuxdeploy`).
- `dpkg-dev` and `python3` (used by the Debian packaging script).

### AppImage

```bash
QT_ROOT_DIR=/path/to/qt6 bash packaging/linux/create-appimage.sh
```

- Reads the version from `CMakeLists.txt` (override the name with
  `APPIMAGE_NAME=...`).
- Downloads `linuxdeploy` and its Qt plugin automatically into `build/.tools`,
  bundles Qt platform plugins (xcb + wayland) and essential QML modules
  (`QtQml.Base`, `QtQml.WorkerScript`), and verifies them before producing the
  image.
- Output: `dist/ZowiDesktop-<version>-x86_64.AppImage`.

### Debian packages

Build the **jammy** package on Ubuntu 22.04 and the **noble** package on
Ubuntu 24.04 (or matching containers) so the dependency metadata and toolchain
match the target distribution:

```bash
DISTRO_SUFFIX=jammy bash packaging/linux/create-deb.sh   # on Ubuntu 22.04
DISTRO_SUFFIX=noble bash packaging/linux/create-deb.sh   # on Ubuntu 24.04
```

- Regenerates `debian/changelog` from `CHANGELOG.md` (top entry uses the
  `CMakeLists.txt` version).
- Builds with `dpkg-buildpackage -b -us -uc`.
- Output: `dist/zowi-desktop_<version>-1+<suffix>_amd64.deb`.

> **Note:** packaging scripts disable `dev_mode` in `src/config.json` and
> restore it afterwards (the Windows scripts back the file up first, the Linux
> scripts use `git checkout`). Do not run them on a tree with uncommitted
> changes to `src/config.json`.

## Windows artifacts

### Option A — GitHub Actions (recommended)

1. Go to **Actions → Windows CI → Run workflow** (manual `workflow_dispatch`).
   The workflow builds on `windows-latest` with MSVC 2022 and Qt 6.8.
2. When it finishes, download the two artifacts:
   - `ZowiDesktop-<version>-windows-x86_64.zip` (portable, **GUI + CLI**)
   - `ZowiDesktop-<version>-setup-x64.exe` (Inno Setup installer)
3. Place them in `dist/` — where the release script looks for
   both Windows artifacts.

### Option B — Local Windows machine

From an **x64 Native Tools Command Prompt for VS 2022** (Qt and CMake on PATH),
same as the normal builds:

```bat
packaging\windows\installer\build-installer.bat    :: installer
packaging\windows\build-portable.bat               :: portable zip
```

- Both scripts read the version from `CMakeLists.txt` and write directly to
  `dist/`:
  - `build-installer.bat` → `dist\ZowiDesktop-<version>-setup-x64.exe`
    (packed from the windeployqt staging dir `build-windows\stage\`)
  - `build-portable.bat` → `dist\ZowiDesktop-<version>-windows-x86_64.zip`
- The local portable zip includes the **GUI + CLI** (`ZOWI_BUILD_CLI=ON`), just
  like the CI zip.
- Packaged builds ship with `dev_mode=false` in the compiled config (re-enable
  at runtime via the `DEV_MODE` environment variable).

## Publish the GitHub Release

### Prerequisites

- `gh` CLI installed and authenticated: `gh auth login`.
- All mandatory Linux artifacts in `dist/` (AppImage + jammy + noble `.deb`).
- Optional Windows artifacts (portable zip + installer) in `dist/`.

### Release notes

`create-gh-release.sh` extracts the release notes from `debian/changelog`, which
`create-deb.sh` regenerates from `CHANGELOG.md` (and then restores via
`git checkout`). To publish the real changelog bullets as release notes,
**commit the regenerated `debian/changelog` before running the release script**:

```bash
git add debian/changelog && git commit -m "docs(debian): regenerate changelog for v<version>"
```

If no matching entry is present, the script falls back to the note
`Release <version>`.

### Create the release

```bash
# Linux artifacts only (+ Windows artifacts if present)
bash packaging/create-gh-release.sh

# Same, and also publish the signed apt repo (jammy + noble) to gh-pages
bash packaging/create-gh-release.sh --with-apt
```

The script:

1. Checks `gh` is authenticated.
2. Reads the version from `CMakeLists.txt` (`TAG="v<version>"`).
3. Verifies the mandatory artifacts exist (AppImage + both `.deb`).
4. Extracts the changelog notes for the version from `debian/changelog`.
5. Confirms interactively, then creates the annotated tag `v<version>` and
   pushes it.
6. Creates the GitHub Release (`gh release create`) with the notes and the
   artifacts attached.
7. With `--with-apt`, runs `packaging/publish-apt-repo.sh` to publish the
   signed apt repository.

The script prints the release URL when it finishes:

```
https://github.com/<owner>/ZowiDesktop/releases/tag/v<version>
```

## Publish the signed apt repository (`--with-apt`)

Only run this from a machine holding the repo's **GPG signing key**:

- Install `aptly` and `gnupg`: `sudo apt-get install aptly gnupg`.
- Import the GPG signing key into your local keyring and unlock it by setting
  `GPG_PASSPHRASE` (or `APTLY_GPG_PASSPHRASE`).

`packaging/publish-apt-repo.sh <version> dist/`:

1. Recreates the `zowi-jammy` and `zowi-noble` aptly repos with the new `.deb`s
   and re-publishes them (publish output goes to `~/.aptly/public`).
2. Stages `docs/{dists,pool,keyring.gpg,.nojekyll}` and pushes them to the
   `gh-pages` branch **under `docs/`**, preserving everything else (`keep_files`
   semantics). The project website lives on the same branch and must not be
   deleted.
3. Installs point users at the apt repository hosted on
   `https://eduardomillan.github.io/ZowiDesktop/docs/`.

## Verify the release

- Install the `.deb` on a clean Ubuntu 22.04 and 24.04 (`sudo apt install
  ./zowi-desktop_<version>-1+<suffix>_amd64.deb`).
- Run the AppImage on a machine without Qt installed.
- On Windows, unzip the portable build and run `ZowiDesktop.exe`; run the
  installer on a clean machine and check the Start Menu / desktop shortcuts.
- Open the release URL printed above and confirm all five artifacts and the
  notes are present; with `--with-apt`, confirm `apt-get update` against the
  published repo works and is GPG-signed.