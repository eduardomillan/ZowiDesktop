# Screen Preview Scripts

This directory contains scripts for previewing individual QML screens in isolation, without needing to navigate through the full application flow.

## Table of Contents

- [Overview](#overview)
- [Usage](#usage)
  - [Generic Preview Script](#generic-preview-script)
  - [Screen-Specific Shortcuts](#screen-specific-shortcuts)
- [Options](#options)
  - [Qt Version Selection](#qt-version-selection)
  - [Screen State Options](#screen-state-options)
  - [Environment Variables](#environment-variables)
- [How It Works](#how-it-works)
- [Available Screens](#available-screens)
- [Adding a New Preview Script](#adding-a-new-preview-script)
- [Troubleshooting](#troubleshooting)
  - [Build Failures](#build-failures)
  - [Screen Not Loading](#screen-not-loading)

## Overview

The preview system allows developers to quickly view and test individual screens during development. Each screen can be launched with specific initial states (e.g., connected/disconnected, different locales) to facilitate rapid iteration.

## Usage

### Generic Preview Script

The main entry point is `preview-screen.sh`, which can preview any screen:

```bash
./preview-screen.sh <ScreenName> [options]
```

**Examples:**
```bash
./preview-screen.sh SplashScreen
./preview-screen.sh HomeScreen --connected
./preview-screen.sh PadScreen --connected --locale es_ES
```

### Screen-Specific Shortcuts

For convenience, each screen has a dedicated shortcut script (e.g., `preview-home.sh`, `preview-pad.sh`). These are simple wrappers that call `preview-screen.sh` with the appropriate screen name and default options.

**Examples:**
```bash
./preview-splash.sh
./preview-home.sh
./preview-pad.sh
./preview-wizard.sh
```

## Options

### Qt Version Selection

- `-5` — Build and run against Qt 5
- `-6` — Build and run against Qt 6 (default)

**Example:**
```bash
./preview-screen.sh PadScreen -5
```

### Screen State Options

These options are forwarded to the `zowi_screen_preview` executable:

- `--connected` — Simulate a connected robot state
- `--locale <locale>` — Set the application locale (e.g., `es_ES`, `fr_FR`, `en_US`)

**Example:**
```bash
./preview-screen.sh PadScreen --connected --locale fr_FR
```

### Environment Variables

- `QT_PATH` — Path to Qt installation (e.g., `~/Qt/6.5.2/gcc_64`)

**Example:**
```bash
QT_PATH=~/Qt/6.5.2/gcc_64 ./preview-screen.sh HomeScreen
```

## How It Works

1. `preview-screen.sh` configures CMake with the appropriate Qt version and build options
2. Builds the `zowi_screen_preview` target (a minimal QML loader)
3. Launches the preview executable with the specified screen path and options

The preview executable loads the QML screen in isolation, providing mock context properties (e.g., `Robot`, `Session`, `Translator`) as needed.

## Available Screens

| Script | Screen | Default Options |
|--------|--------|-----------------|
| `preview-splash.sh` | SplashScreen | — |
| `preview-welcome.sh` | WelcomeScreen | — |
| `preview-wizard.sh` | WizardScreen | — |
| `preview-wizardfound.sh` | WizardFoundScreen | — |
| `preview-wizardrename.sh` | WizardRenameScreen | — |
| `preview-scan.sh` | ScanScreen | — |
| `preview-home.sh` | HomeScreen | `--connected` |
| `preview-pad.sh` | PadScreen | `--connected` |

## Adding a New Preview Script

To create a preview script for a new screen:

1. Create a new file `preview-<screenname>.sh` in this directory
2. Use the following template:

```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$SCRIPT_DIR/preview-screen.sh" YourScreenName "$@"
```

3. Make it executable: `chmod +x preview-<screenname>.sh`
4. Add default options if needed (e.g., `--connected` for screens that require a robot connection)

## Troubleshooting

### Build Failures

If the preview fails to build, ensure:
- Qt is properly installed and `QT_PATH` is set if needed
- The screen QML file exists in `src/views/screens/`
- All dependencies are available

### Screen Not Loading

If the screen loads but appears blank or shows errors:
- Check that the screen name matches the QML file name (case-sensitive)
- Verify that required context properties are provided (use `--connected` if needed)
- Check the console output for QML errors
