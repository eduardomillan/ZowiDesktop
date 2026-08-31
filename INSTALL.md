# Installing Zowi Desktop

## Option A — standalone download (AppImage / .deb)

Grab the latest `ZowiDesktop-*.AppImage` or `zowi-desktop_*.deb` from the
[Releases](https://github.com/eduardomillan/ZowiDesktop/releases) page.

- **AppImage**: `chmod +x ZowiDesktop-*.AppImage && ./ZowiDesktop-*.AppImage`
- **Debian / Lliurex / Ubuntu**: `sudo apt install ./zowi-desktop_*.deb`

## Option B — apt repository (recommended, auto-updates)

```bash
sudo curl -fsSL https://eduardomillan.github.io/ZowiDesktop/keyring.gpg \
  -o /usr/share/keyrings/zowi-desktop-archive-keyring.gpg

# Pick the suite that matches your base:
#   Lliurex 23 / Ubuntu 22.04 -> jammy
#   Lliurex 25 / Ubuntu 24.04 -> noble
DISTRO=jammy   # change to "noble" on Lliurex 25

sudo echo "deb [signed-by=/usr/share/keyrings/zowi-desktop-archive-keyring.gpg] \
  https://eduardomillan.github.io/ZowiDesktop $DISTRO main" \
  | sudo tee /etc/apt/sources.list.d/zowi-desktop.list
sudo apt update && sudo apt install zowi-desktop
```

Releases are created manually as GitHub Releases with the AppImage and
Debian packages attached.

## Windows

Windows builds are attached to each Release as a portable `.zip` and an
installer `.exe`. Both are 64-bit and require a recent Windows 10 or 11.

- **Installer (recommended)**: download `ZowiDesktop-<version>-setup-x64.exe`
  and run it. It installs for all users (admin rights required) and adds
  Start-menu shortcuts. Uninstall from *Settings → Apps*.
- **Portable**: download `ZowiDesktop-<version>-windows-x86_64.zip`, extract it
  anywhere, and run `ZowiDesktop.exe` — no installation needed.

> Note: Windows builds are produced on demand (see `.github/workflows/windows.yml`)
> and attached to the release manually, so they may appear after the Linux
> assets on a given release.

