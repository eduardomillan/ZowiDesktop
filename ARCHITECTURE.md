# Architecture

## Overview

Zowi Desktop follows a **Model-View-ViewModel (MVVM)** pattern using Qt 6 / QML. This allows:

- Iterating on QML screens without recompiling C++
- Reactive data binding between View and ViewModel
- Unit-testing business logic in isolation
- Clean separation between UI, presentation logic, and services

## Why MVVM (not MVC)

Qt/QML is inherently MVVM. In MVC, the Controller handles input and manually updates both View and Model. In MVVM, the View **declaratively binds** to ViewModel properties — when the ViewModel changes, the View updates automatically. This is exactly how QML data binding works:

```qml
// View binds to ViewModel property — no imperative update needed
opacity: Translator.currentLocale() === modelData.locale ? 1.0 : 0.4
```

```cpp
// ViewModel exposes property via Q_INVOKABLE
Q_INVOKABLE QString currentLocale() const;
```

### Pattern comparison

| Pattern | View ↔ Logic relationship | Qt/QML fit |
|---------|--------------------------|------------|
| MVC | Controller handles input, manually updates View | Poor — requires imperative bridge |
| MVP | Presenter drives View through interface | Moderate — View must be passive |
| **MVVM** | **View binds to ViewModel properties; ViewModel updates state** | **Natural — QML is declarative binding** |

### Layer mapping

| MVVM role | Zowi Desktop layer | Directory | Exposed to QML? |
|-----------|-------------------|-----------|-----------------|
| **Model** | Services | `services/` | No |
| **ViewModel** | Controllers | `controllers/` | Yes (as `contextProperty`) |
| **View** | QML screens | `views/` | — |

The `controllers/` directory could be renamed to `viewmodels/` for stricter MVVM nomenclature, but functionally they are ViewModels already.

## Directory Layout

```
src/
├── main.cpp                        # QML engine setup, context property wiring
├── config.json                     # App config (image paths, URLs)
├── core/
│   └── DeviceInfo.h                # Plain data struct (name, address, rssi)
├── services/                       # Model layer — business logic
│   ├── BluetoothService.h/.cpp     #   Discovery + SPP socket
│   ├── BluetoothNativeLoader.h/.cpp#   Dynamic DLL loader for Windows native BT
│   ├── SessionService.h/.cpp       #   QSettings persistence
│   └── TranslationEngine.h/.cpp    #   JSON-based i18n engine
├── controllers/                    # ViewModel layer — QML bridge
│   ├── BluetoothController.h/.cpp  #   Exposes BT state + commands
│   ├── ConfigController.h/.cpp     #   Exposes config.json values
│   ├── SessionController.h/.cpp    #   Exposes session load/save
│   └── TranslatorController.h/.cpp #   Exposes translations + locale
├── views/                          # View layer — QML screens
│   ├── main.qml                    #   Root window, StackView navigation
│   ├── screens/
│   │   ├── SplashScreen.qml        #   Logo + language flags + Continue/Quit
│   │   ├── WelcomeScreen.qml       #   Start button + know-more link
│   │   ├── WizardScreen.qml        #   Onboarding: "Do you have a Zowi?"
│   │   └── ScanScreen.qml          #   Bluetooth device discovery
│   └── components/
│       └── AnimatedZowi.qml        #   Reusable animated sprite
├── tests/
│   ├── CMakeLists.txt
│   ├── tst_SessionService.cpp
│   └── tst_TranslationEngine.cpp
└── packaging/
    └── windows/
        └── bt_native/              # Native Windows Bluetooth DLL
            ├── bt_native.h         #   C API header
            ├── bt_native.cpp       #   WinRT implementation (stub)
            ├── bt_native.def       #   DLL exports
            ├── CMakeLists.txt      #   MSVC build
            └── README.md           #   Compilation instructions
```

## MVVM Data Flow

```
┌──────────┐   data binding /     ┌──────────────────┐   method calls   ┌──────────────┐
│          │   Q_INVOKABLE        │                  │ ──────────────── │              │
│   QML    │ ───────────────────► │    ViewModel     │                  │    Model     │
│  (View)  │                     │  (Controller)    │                  │  (Service)   │
│          │ ◄─────────────────── │                  │ ◄─────────────── │              │
│          │   property changes   │                  │   signals        │              │
└──────────┘   via bindings       └──────────────────┘                  └──────┬───────┘
                                                                              │
                                                                       ┌──────▼───────┐
                                                                       │  Core (Data) │
                                                                       │  DeviceInfo  │
                                                                       └──────────────┘
```

1. **View** (QML) binds to ViewModel properties or calls `Q_INVOKABLE` methods
2. **ViewModel** translates requests into Model (Service) calls
3. **Model** (Service) executes business logic, updates state
4. **Model** emits `signal` → ViewModel relays → View re-binds reactively

**Hard rule:** QML never accesses Services or Models directly. Always through the ViewModel.

## Layer Responsibilities

### Core (data structs)

- Plain structs or simple classes, minimal `QObject` usage
- Data only, zero business logic
- Example: `DeviceInfo` with `name`, `address`, `rssi`

### Model / Services

- Real business logic: Bluetooth discovery/connection, persistence, translation
- No Qt Quick / QML dependency
- May emit signals (`QObject` subclass)
- Independent of UI — testable in isolation

### ViewModel / Controllers

- `QObject` subclass exposed to QML via `setContextProperty`
- Translate QML requests into Service calls
- Expose `Q_PROPERTY` and `Q_INVOKABLE` for reactive binding
- No business logic; orchestration and signal relay only

### View (QML)

- Presentation only: layouts, colors, animations, text
- No business logic: no calculations, no model management
- Screen navigation handled by `main.qml` StackView

## Constructor Wiring

In `main.cpp`, Services are created first, injected into ViewModels, then ViewModels are exposed to QML:

```
TranslationEngine  ──► TranslatorController  ──► QML context "Translator"
SessionService     ──► SessionController     ──► QML context "Session"
BluetoothService   ──► BluetoothController   ──► QML context "Bluetooth"
(config.json)      ──► ConfigController      ──► QML context "Config"
```

## QML Hot-Reload

In **debug** mode, QML is loaded from the filesystem. A `QFileSystemWatcher` monitors `src/views/` and triggers a full engine reload on any `.qml` change:

```cpp
#ifdef QT_DEBUG
    s_qmlPath = QUrl::fromLocalFile("src/views/main.qml").toString();
#else
    s_qmlPath = "qrc:/src/views/main.qml";
#endif
```

Language changes also trigger `reloadQml()` to re-translate all screens.

## Library Build Targets

| Library            | Depends On                   | Rebuilt when                      |
|--------------------|------------------------------|-----------------------------------|
| `zowi_core`        | Qt6::Core (header-only)      | Core data types change            |
| `zowi_services`    | Qt6::Core, Qt6::Bluetooth    | Business logic changes            |
| `zowi_controllers` | Qt6::Core, Qt6::Quick, Qt6::QuickControls2 | ViewModel / QML interface changes |
| `ZowiDesktop` (exe)| links all three              | Only `main.cpp` changes           |

> **Note:** On Windows, `zowi_services` also includes `BluetoothNativeLoader.cpp`
> (conditionally compiled via `if(WIN32)` in CMakeLists.txt).

## Screen Navigation

Navigation uses a `StackView` in `main.qml`:

```
SplashScreen → WelcomeScreen → WizardScreen → ScanScreen
                    ↑              ↓
                    └──────────────┘  (goBack)
```

## View Mapping (Android → Desktop)

ZowiDesktop is the desktop port of [ZowiAppReborn](https://github.com/eduardomillan/ZowiAppReborn) (Android). Views are mapped to maintain naming consistency with the original project.

### Completed screens

| Android Activity | Android Path | Desktop QML | Status |
|---|---|---|---|
| `SplashViewActivity` | `views/splash/` | `SplashScreen.qml` | ✅ |
| `WelcomeViewActivity` | `views/welcome/` | `WelcomeScreen.qml` | ✅ |
| `WizardViewActivity` | `views/wizard/` | `WizardScreen.qml` + `ScanScreen.qml` | ✅ |
| — | — | `StartScreen.qml` | ❌ Removed (merged into WelcomeScreen) |

### Planned screens (M3–M8)

| Android Activity | Android Path | Desktop QML | Milestone |
|---|---|---|---|
| `HomeViewActivity` | `views/interactive/home/` | `HomeScreen.qml` | M3 |
| `PadViewActivity` | `views/interactive/pad/` | `PadScreen.qml` | M3 |
| `TimelineActivity` | `views/interactive/timeline/` | `TimelineScreen.qml` | M3 |
| `AchievementsViewActivity` | `views/interactive/achievements/` | `AchievementsScreen.qml` | M7 |
| `ProjectViewActivity` | `views/interactive/projects/` | `ProjectScreen.qml` | M7 |
| `ProjectQuizViewActivity` | `views/interactive/projects/` | `ProjectQuizScreen.qml` | M7 |
| `SettingsViewActivity` | `views/interactive/settings/` | `SettingsScreen.qml` | M8 |
| `CalibrationViewActivity` | `views/interactive/settings/` | `CalibrationScreen.qml` | M8 |
| `MouthsEditorActivity` | `views/interactive/zowiapps/` | `MouthsEditorScreen.qml` | M6 |

### Naming convention

- **Android:** `{Name}ViewActivity` (e.g., `WizardViewActivity`)
- **Desktop:** `{Name}Screen.qml` (e.g., `WizardScreen.qml`)

### Desktop-only additions

| Screen | Description | Notes |
|---|---|---|
| `ScanScreen.qml` | Bluetooth device discovery (part of Wizard flow) | In Android, scanning is inside WizardViewActivity; on desktop it's a separate screen |

### Android-only screens (not planned for desktop)

| Screen | Description | Reason |
|---|---|---|
| `ZowiSaysMinigameActivity` | Zowi Says mini-game | M4 scope; may be deferred |
| `MouthsMinigameActivity` | Mouths mini-game | M4 scope; may be deferred |
| `ZowiRunnerMinigameActivity` | Zowi Runner mini-game | M4 scope; may be deferred |

## When to Add a New Screen

1. Create the `.qml` file in `views/screens/`
2. If it needs data from a Service, add a `Q_PROPERTY` / `Q_INVOKABLE` to the relevant **ViewModel** (never access a Service from QML)
3. If it needs new business logic, add it to the relevant **Service** (never put logic in a ViewModel)
4. Add the screen to `main.qml` StackView navigation
