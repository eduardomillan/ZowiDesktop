# GitHub Actions Workflows

All workflows in this repository are **manual (`workflow_dispatch`)** — there
are no automatic triggers on push or tag. This is consistent with the project's
manual-release philosophy.

Two of the build workflows (`linux.yml` and `windows.yml`) are also
**reusable (`workflow_call`)**, meaning the `release.yml` workflow can invoke
them as jobs without duplicating logic.

## Table of contents

- [Linux CI](#linux-ci)
- [Windows CI](#windows-ci)
- [Tests](#tests)
- [Tests (Windows)](#tests-windows)
- [Release](#release)

---

## Linux CI

**File:** `.github/workflows/linux.yml`

Builds the Linux release artifacts: AppImage and Debian packages for Ubuntu 22.04
(Jammy) and Ubuntu 24.04 (Noble).

### How to run

**Actions → Linux CI → Run workflow**

### Inputs

| Input | Type | Default | Description |
|-------|------|---------|-------------|
| `build_appimage` | boolean | `true` | Build the Linux AppImage |
| `build_deb_jammy` | boolean | `true` | Build Debian package for Ubuntu 22.04 (Jammy) |
| `build_deb_noble` | boolean | `true` | Build Debian package for Ubuntu 24.04 (Noble) |

### What it does

Three parallel jobs, one per artifact:

- **appimage** (`ubuntu-latest`): installs Qt 6.8, runs
  `packaging/linux/create-appimage.sh`. Produces
  `dist/ZowiDesktop-<version>-x86_64.AppImage`.
- **deb-jammy** (`ubuntu-22.04`): installs Qt 6 system packages + build
  deps, runs `DISTRO_SUFFIX=jammy packaging/linux/create-deb.sh`. Produces
  `dist/zowi-desktop_<version>-1+jammy_amd64.deb`.
- **deb-noble** (`ubuntu-24.04`): same as above with `DISTRO_SUFFIX=noble`.
  Produces `dist/zowi-desktop_<version>-1+noble_amd64.deb`.

Each job uploads its artifact to GitHub Actions. Download them and place them
in `dist/` for use with `packaging/create-gh-release.sh`.

### Example usage

Build only the AppImage (skip both `.deb` packages):

```
Actions → Linux CI → Run workflow
  build_appimage:  ✓
  build_deb_jammy: ✗
  build_deb_noble: ✗
```

---

## Windows CI

**File:** `.github/workflows/windows.yml`

Builds the Windows release artifacts: a portable `.zip` and an Inno Setup
installer (`.exe`). Uses MSVC 2022 + Windows SDK on `windows-2022`.

### How to run

**Actions → Windows CI → Run workflow**

### Inputs

| Input | Type | Default | Description |
|-------|------|---------|-------------|
| `build_zip` | boolean | `true` | Build portable `.zip` (GUI + CLI + Qt DLLs) |
| `build_installer` | boolean | `true` | Build Inno Setup `.exe` installer |

### What it does

A single `windows` job on `windows-2022`:

1. Installs Qt 6.8 (MSVC) and Inno Setup 6.
2. Disables `dev_mode` in `src/config.json` for the packaged build.
3. Configures CMake with Visual Studio 17 2022 generator + MSVC x64.
4. Builds GUI + CLI in Release mode.
5. Runs `windeployqt` to stage Qt DLLs and QML files.
6. Creates the portable `.zip` with 7z and the installer with `ISCC.exe`.
7. Uploads both artifacts.

### Example usage

Build only the installer (skip the portable zip):

```
Actions → Windows CI → Run workflow
  build_zip:       ✗
  build_installer: ✓
```

---

## Tests

**File:** `.github/workflows/tests.yml`

Runs the automated test suites on Linux — white-box unit tests (via CTest) and
backend-agnostic black-box CLI tests. Hardware-dependent tests (real Bluetooth
or USB robot) are **not** run in CI.

### How to run

**Actions → Tests → Run workflow**

### Inputs

None — this workflow takes no parameters.

### What it does

A single `linux` job on `ubuntu-latest`:

1. Installs Qt 6.8 with `qtconnectivity`.
2. Configures CMake for CLI-only build (`ZOWI_BUILD_GUI=OFF`, `ZOWI_BUILD_CLI=ON`).
3. Builds the `zowi_cli` target.
4. Runs `ctest --test-dir build --output-on-failure`, which executes both
   white-box unit tests (`src/core/tests/`) and black-box CLI tests
   (`cli_blackbox`).

For hardware-dependent tests (Bluetooth/USB with a real robot), run locally:

```bash
scripts/test/run-cli-blackbox.sh          # opt-in black-box
src/cli/tests/bt/run_all.sh              # Bluetooth tests
src/cli/tests/usb/run_all.sh             # USB tests
```

---

## Tests (Windows)

**File:** `.github/workflows/tests-windows.yml`

Runs the same test suites as [Tests](#tests) but on Windows with the MSVC
toolchain. Complements the Linux workflow to verify cross-platform correctness.

### How to run

**Actions → Tests (Windows) → Run workflow**

### Inputs

None — this workflow takes no parameters.

### What it does

A single `windows` job on `windows-latest`:

1. Installs Qt 6.8 (MSVC).
2. Configures CMake with Visual Studio 17 2022 generator.
3. Builds the CLI target (`zowi_cli`) in Release mode.
4. Runs `ctest --test-dir build -C Release --output-on-failure`.

Black-box tests here only cover platform-stable cases (help, session, config,
translate, empty-session failure paths). Bluetooth/USB reachability tests are
omitted because the Windows backend (`bt_native`/WinSerial) produces different
output than Linux/BlueZ.

---

## Release

**File:** `.github/workflows/release.yml`

End-to-end, one-button release. Builds all selected platform artifacts, creates
the GitHub Release (tag + notes + assets), and optionally publishes the signed
apt repository to `gh-pages`.

### How to run

**Actions → Release → Run workflow**

### Inputs

| Input | Type | Default | Description |
|-------|------|---------|-------------|
| `include_appimage` | boolean | `true` | Linux: include the AppImage |
| `include_deb_jammy` | boolean | `true` | Linux: include Ubuntu 22.04 Jammy `.deb` |
| `include_deb_noble` | boolean | `true` | Linux: include Ubuntu 24.04 Noble `.deb` |
| `include_windows_zip` | boolean | `true` | Windows: include portable `.zip` |
| `include_windows_installer` | boolean | `true` | Windows: include setup `.exe` installer |
| `publish_apt` | boolean | `false` | Publish signed apt repo (jammy + noble) to `gh-pages` |
| `overwrite` | boolean | `false` | Overwrite existing release and tag if they exist |
| `prerelease` | boolean | `false` | Mark the GitHub Release as a pre-release (not *latest*) |

### What it does

Three sequential phases:

1. **Build Linux artifacts** — calls [Linux CI](#linux-ci) as a reusable
   workflow with the corresponding `build_*` flags.
2. **Build Windows artifacts** — calls [Windows CI](#windows-ci) as a reusable
   workflow with the corresponding `build_*` flags.
3. **Create GitHub Release** — downloads all produced artifacts into `dist/`,
   then runs `packaging/create-gh-release.sh` with the appropriate `--skip-*`
   flags (derived from which inputs were unchecked or whose builds failed).

When `prerelease` is enabled:

- The release is created with `gh release create --prerelease`: GitHub shows
  it as a **pre-release** and does **not** mark it as *latest* (a normal run
  is marked *latest* automatically).
- Cannot be combined with `publish_apt`: the signed apt repo is the stable
  channel and must not carry pre-release packages (the script fails with an
  error if both are requested).

When `publish_apt` is enabled:

- Requires repository secrets `APT_GPG_PRIVATE_KEY` (base64-encoded) and
  `APT_GPG_PASSPHRASE`.
- Both `include_deb_jammy` and `include_deb_noble` must be enabled.
- The GPG key is imported into the runner's ephemeral keyring, used for
  signing, and deleted at the end.
- Runs `packaging/publish-apt-repo.sh` to publish the signed apt repository
  under `docs/` on `gh-pages`.

### Example usage

Full release with apt repo publishing:

```
Actions → Release → Run workflow
  include_appimage:        ✓
  include_deb_jammy:       ✓
  include_deb_noble:       ✓
  include_windows_zip:     ✓
  include_windows_installer: ✓
  publish_apt:             ✓
  overwrite:               ✗
```

Linux-only release (no Windows artifacts):

```
Actions → Release → Run workflow
  include_appimage:        ✓
  include_deb_jammy:       ✓
  include_deb_noble:       ✓
  include_windows_zip:     ✗
  include_windows_installer: ✗
  publish_apt:             ✗
  overwrite:               ✗
```

Re-run a failed release, replacing the existing tag:

```
Actions → Release → Run workflow
  (all inputs as needed)
  overwrite:               ✓
```

Test release that must not become *latest* (no apt publishing):

```
Actions → Release → Run workflow
  (artifact inputs as needed)
  publish_apt:             ✗
  prerelease:              ✓
```

### Prerequisites

- Repository secrets must be configured (Settings → Secrets and variables →
  Actions) if using `publish_apt`:
  - `APT_GPG_PRIVATE_KEY` — base64-encoded exported private key
  - `APT_GPG_PASSPHRASE` — passphrase for the key
- The `GITHUB_TOKEN` (automatically available) is used for creating the release
  and pushing the tag.
