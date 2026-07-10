# Architecture

## Overview

Zowi Desktop follows a **Model-View-Controller (MVC)** pattern to keep UI, logic, and data independent. This allows:

- Iterating on QML screens without recompiling C++
- Unit-testing business logic in isolation
- Building in layers so changes propagate minimally

## Directory Layout

```
src/
├── core/                          # Models — pure data structures
│   └── DeviceInfo.h               #   Bluetooth device descriptor
├── services/                      # Business logic / backend
│   ├── BluetoothService.h/.cpp    #   Discovery + SPP socket
│   └── SessionService.h/.cpp      #   QSettings persistence
├── controllers/                   # Bridge between models and QML
│   ├── BluetoothController.h/.cpp
│   ├── SessionController.h/.cpp
│   └── TranslatorController.h/.cpp
├── views/                         # QML screens (loaded from disk in debug)
│   ├── main.qml
│   ├── screens/
│   │   ├── SplashScreen.qml
│   │   ├── WelcomeScreen.qml
│   │   └── ScanScreen.qml
│   └── components/
│       └── AnimatedZowi.qml
├── main.cpp
└── tests/
    ├── CMakeLists.txt
    ├── tst_SessionService.cpp
    └── tst_DeviceInfo.cpp
```

## MVC Data Flow

```
┌──────────┐   Q_PROPERTY /      ┌──────────────┐   method calls    ┌────────────────┐
│          │   Q_INVOKABLE       │              │ ────────────────── │                │
│   QML    │ ──────────────────► │  Controller  │                   │    Service     │
│  (View)  │                     │  (Qt Object) │ ─ ─ ─ ─ ─ ─ ─ ─ ► │  (Business     │
│          │ ◄────────────────── │              │   signals         │   Logic)       │
│          │     signals         │              │ ◄─ ─ ─ ─ ─ ─ ─ ─  │                │
└──────────┘                     └──────────────┘                   └───────┬────────┘
                                                                           │
                                                                    ┌──────▼────────┐
                                                                    │    Model(s)   │
                                                                    │  (Data only)  │
                                                                    └───────────────┘
```

1. **View** (QML) invokes Controller methods or binds to `Q_PROPERTY`
2. **Controller** translates the request into Service calls
3. **Service** executes business logic and updates Models or internal state
4. **Controller** emits a `signal` → QML reactively refreshes

**Hard rule:** QML never accesses Services or Models directly. Always through the Controller.

## Layer Responsibilities

### Core (models)

- Plain structs or simple classes without `QObject` or signals
- Data only, zero business logic
- Minimal dependency: `Qt5::Core` (QString, etc.)

### Services

- Real business logic: Bluetooth discovery/connection, persistence, translation
- No Qt Quick / QML dependency
- May emit signals (`QObject` subclass)

### Controllers

- `QObject` subclass exposed to the QML context
- Translate QML requests into Service calls
- Expose `Q_PROPERTY` for reactive binding
- No business logic; orchestration only

### Views (QML)

- Presentation only: layouts, colors, animations, text
- No business logic: no calculations, no model management
- Screen navigation is handled from `main.qml` / Controller

## QML Hot-Reload

In **debug** mode, QML is loaded from the filesystem so you can edit without recompiling:

```cpp
#ifdef QT_DEBUG
    engine.load(QUrl::fromLocalFile("src/views/main.qml"));
#else
    engine.load(QUrl("qrc:/views/main.qml"));
#endif
```

To launch the app in development mode:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -B build && cmake --build build
./build/ZowiDesktop
```

Any `.qml` change takes effect when the binary restarts (no C++ recompilation needed).

## Library Packaging

The project is split into three static libraries for incremental compilation:

| Library           | Depends On                  | Rebuilt when                    |
|-------------------|-----------------------------|---------------------------------|
| `zowi_core`       | Qt5::Core                   | Models / core types change      |
| `zowi_services`   | Qt5::Core, Qt5::Bluetooth   | Business logic changes          |
| `zowi_controllers`| Qt5::Core, Qt5::Quick       | Controller / View interface chg |
| `ZowiDesktop` (exe)| links all three            | Only `main.cpp` changes         |

```cmake
# CMakeLists.txt (high-level)
add_library(zowi_core STATIC
    core/DeviceInfo.h
)
target_link_libraries(zowi_core PUBLIC Qt5::Core)

add_library(zowi_services STATIC
    services/BluetoothService.h services/BluetoothService.cpp
    services/SessionService.h   services/SessionService.cpp
)
target_link_libraries(zowi_services PUBLIC Qt5::Core Qt5::Bluetooth)

add_library(zowi_controllers STATIC
    controllers/BluetoothController.h   controllers/BluetoothController.cpp
    controllers/SessionController.h     controllers/SessionController.cpp
    controllers/TranslatorController.h  controllers/TranslatorController.cpp
)
target_link_libraries(zowi_controllers PUBLIC Qt5::Core Qt5::Quick zowi_core zowi_services)

add_executable(ZowiDesktop main.cpp ${RESOURCES})
target_link_libraries(ZowiDesktop PRIVATE zowi_controllers)
```

## Testing

Tests use **QtTest** (shipped with Qt). They are compiled as standalone executables linking only the libraries they need.

```cmake
# src/tests/CMakeLists.txt
add_executable(tst_SessionService
    tst_SessionService.cpp
)
target_link_libraries(tst_SessionService PRIVATE zowi_services Qt5::Test)

add_test(NAME tst_SessionService COMMAND tst_SessionService)
```

Run:

```bash
cmake --build build && ctest --test-dir build --output-on-failure
```

### Mocking

For services with external dependencies (Bluetooth), define a virtual interface in `core/` and inject a mock in tests. Example:

```cpp
// core/IBluetoothService.h  (interface)
class IBluetoothService {
public:
    virtual ~IBluetoothService() = default;
    virtual void startScan() = 0;
    virtual void connectToDevice(const QString &address) = 0;
    // ...
};

// services/BluetoothService.h  (implementación real)
class BluetoothService : public QObject, public IBluetoothService { ... };

// tests/mocks/MockBluetoothService.h
class MockBluetoothService : public IBluetoothService { ... };
```

## Current Status vs Target

| Aspect | Current | Target |
|--------|---------|--------|
| QML loading | Embedded in `resources.qrc` | Filesystem (debug) / resources (release) |
| Model layer | Mixed into controllers | Separate `core/` structs |
| Testing | None | QtTest per service layer |
| Build | Single executable | 3 static libs + executable |
| Bluetooth | Tight coupling in controller | Service with injectable interface |
