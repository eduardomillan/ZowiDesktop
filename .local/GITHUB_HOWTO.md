# GitHub How-To

## Repository context

- Repository: [eduardomillan/ZowiDesktop](https://github.com/eduardomillan/ZowiDesktop)
- Description: This is a reborn project for the original Zowi App from BQ and the famous robot from Clan TV.
- Language composition:
  - C++: 58.6%
  - QML: 25.8%
  - Shell: 10.9%
  - CMake: 2.3%
  - Batchfile: 1.8%
  - Inno Setup: 0.3%
  - Other: 0.3%

## Quick explanation

### Releases
Releases are the official published versions of the software.
- In ZowiDesktop, the version comes from `CMakeLists.txt`.
- Releases are created manually.
- They usually include Linux AppImage and Debian packages, plus Windows artifacts when available.
- Users download releases from the GitHub Releases page.

### Deployments
Deployments are releases or builds published to a specific environment.
- In a project like ZowiDesktop, deployments can correspond to publishing the signed APT repository or making a build available in a target environment.
- They are about where the software is made available, not just the version itself.

### Packages
Packages are installable or reusable build artifacts.
- In ZowiDesktop, the main packages are `.deb` files for Debian/Ubuntu/Lliurex.
- Packages help users install and update the software.
- A GitHub Release may attach packages, but the package itself is the installable artifact.

## Repo-specific release flow

1. Bump the version in `CMakeLists.txt`.
2. Update `CHANGELOG.md`.
3. Build Linux artifacts: AppImage and Debian packages.
4. Build Windows artifacts: portable zip and installer.
5. Create the GitHub Release.
6. Optionally publish the signed APT repository.

## Installation options mentioned in the repo

- Standalone download from GitHub Releases:
  - AppImage
  - `.deb`
  - Windows portable `.zip`
  - Windows installer `.exe`
- APT repository for Linux auto-updates

## Relevant documentation

- `README.md`
- `INSTALL.md`
- `docs/project/BUILD.md`
- `docs/project/RELEASE.md`
- `docs/project/PLANNING.md`
