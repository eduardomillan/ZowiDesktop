# MVC vs MVP vs MVVM

## Table of contents

- [MVC (Model-View-Controller)](#mvc-model-view-controller)
- [MVP (Model-View-Presenter)](#mvp-model-view-presenter)
- [MVVM (Model-View-ViewModel)](#mvvm-model-view-viewmodel)
  - [Why MVVM fits Qt/QML naturally](#why-mvvm-fits-qtqml-naturally)
  - [Comparison table](#comparison-table)
- [In Zowi Desktop](#in-zowi-desktop)
- [Restaurant analogy](#restaurant-analogy)

## MVC (Model-View-Controller)

The **Controller** is the "boss" that receives user commands (clicks, keystrokes). It interprets the command, tells the **Model** to do the work, and once done, **tells the View to update** by displaying the new data.

- *The View is "dumb" and passive*: it just waits for the Controller to tell it what to show.

**Data flow:** User → Controller → Model → **Controller** → View

**Relationship:** The View doesn't know the Controller exists. The Controller knows the View and manipulates it directly.

**Where it's used:** Classic web frameworks (Django, Ruby on Rails, Spring).

---

## MVP (Model-View-Presenter)

The **Presenter** is the "brain", but here the **View is active**. The user interacts directly with the View, and it **notifies the Presenter** of what happened. The Presenter talks to the Model, processes the data, and **returns it to the View** so it can update itself.

- *The View is "smart" and active*: it asks the Presenter for data and decides how to render it.

**Data flow:** User → View → Presenter → Model → **Presenter** → View

**Relationship:** The View knows the Presenter. The Presenter doesn't know the View directly; it works with an "interface" (a contract) the View must fulfill.

**Where it's used:** Desktop and mobile apps (Android, Xamarin, WinForms).

**Key advantage:** Since the Presenter doesn't know the real View, you can test all application logic with a simulated View — no physical screen needed.

---

## MVVM (Model-View-ViewModel)

The **ViewModel** is an abstraction of the View that exposes **data and commands** through bindable properties. The View **declaratively binds** to these properties — when the ViewModel changes, the View updates automatically. No imperative "push" needed.

- *The View is declarative*: it describes *what* to show based on ViewModel state, not *how* to update.
- *The ViewModel is UI-agnostic*: it doesn't know the View exists, only exposes state and actions.

**Data flow:** User → View → ViewModel → Model → **ViewModel** → View (via data binding)

**Relationship:** The View binds to ViewModel properties. The ViewModel has no reference to the View. The Model has no reference to either.

**Where it's used:** Qt/QML, WPF, Knockout.js, Angular, SwiftUI, Jetpack Compose.

### Why MVVM fits Qt/QML naturally

QML is a declarative binding language. When you write:

```qml
// StatusBar.qml (simplified)
connected: Robot.connected
battery: Robot.battery
// StatusBarView.qml derives the colour and face from those properties
```

The View automatically re-evaluates when `Robot` emits `connectionChanged` or `batteryChanged`. There's no imperative `setColor()` call — the binding handles it. This is the essence of MVVM.

In MVC, the Controller would need to manually call `view.setColor(...)`. In MVP, the Presenter would call `view.showLowBattery()`. In MVVM, neither exists — the binding does the work.

### Comparison table

| Characteristic | **MVC** | **MVP** | **MVVM** |
| :--- | :--- | :--- | :--- |
| **View updates** | Controller pushes to View | Presenter pushes to View | View binds to ViewModel (automatic) |
| **View knows logic?** | No (Controller is hidden) | Yes (calls Presenter) | No (binds to ViewModel) |
| **Logic knows View?** | Yes (manipulates it) | No (uses interface) | No (no reference) |
| **Testability** | Hard (depends on View) | Easy (mock the interface) | Easy (ViewModel is pure logic) |
| **Data binding** | Manual | Manual | Declarative |
| **Boilerplate** | Low | Medium (interface per View) | Low (properties + binding) |

---

## In Zowi Desktop

The desktop app is MVVM-shaped, while the original Android app (ZowiAppReborn) is **MVP** (an `Activity` + `Presenter` + `Wireframe` per screen, with the presenter pushing to the view through an interface):

| Role | Where it lives |
| :--- | :--- |
| **Model** | `src/core/` (`SessionStore`, `RankingStore`, `TimelinePlayer`, `ZowiDiceGame`, …): pure C++, no Qt, tested without a GUI |
| **ViewModel** | `src/gui/controllers/` (`RobotController`, `RankingController`, `ProjectsController`, …): thin `QObject` adapters that expose the model as `Q_PROPERTY` + `Q_INVOKABLE` + signals, registered as context properties (`Robot`, `Ranking`, `Projects`, …) |
| **View** | `src/views/` (QML): declarative bindings to those properties; screens only emit signals and `main.qml` decides the navigation |

Examples of the pattern: `Robot.batteryLow` (a property) drives the colour and the "Low battery!" text of the status bar with no code in the view, and `Robot.lowBatteryDetected(level)` (a signal) is the only thing `main.qml` listens to in order to show the one-off notice. The business rules stay in the core (e.g. `zowi::isValidRobotName()` for names); QML keeps only a mirror to clean the field while typing.

Rule of thumb (also in `AGENTS.md`): the GUI is an adapter layer, not a second business-logic layer; logic that can live in `src/core/` goes there. See [ARCHITECTURE.md](ARCHITECTURE.md) for the layers and the history of the MVVM → API + CLI + Frontend migration.

---

## Restaurant analogy

- **MVC** — The **manager (Controller)** takes your order, goes to the kitchen (Model), cooks, and **brings the plate** to your table (View) himself.

- **MVP** — The **waiter (View)** takes your order and tells the **manager (Presenter)**. The manager cooks (Model) and **hands the plate to the waiter**, who decides how to present it. The manager never sees you.

- **MVVM** — The **menu board (View)** shows what's available based on what the **kitchen (ViewModel)** has prepared. You pick what you want, the kitchen prepares it, and the menu board **updates itself** to reflect what's in stock. Nobody "tells" the board to update — it just reacts.
