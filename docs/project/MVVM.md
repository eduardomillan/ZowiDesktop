# MVC vs MVP vs MVVM

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
opacity: Translator.currentLocale() === modelData.locale ? 1.0 : 0.4
```

The View automatically re-evaluates when `currentLocale()` changes. There's no imperative `setOpacity()` call — the binding handles it. This is the essence of MVVM.

In MVC, the Controller would need to manually call `view.setOpacity(...)`. In MVP, the Presenter would call `view.updateHighlight(...)`. In MVVM, neither exists — the binding does the work.

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

## Restaurant analogy

- **MVC** — The **manager (Controller)** takes your order, goes to the kitchen (Model), cooks, and **brings the plate** to your table (View) himself.

- **MVP** — The **waiter (View)** takes your order and tells the **manager (Presenter)**. The manager cooks (Model) and **hands the plate to the waiter**, who decides how to present it. The manager never sees you.

- **MVVM** — The **menu board (View)** shows what's available based on what the **kitchen (ViewModel)** has prepared. You pick what you want, the kitchen prepares it, and the menu board **updates itself** to reflect what's in stock. Nobody "tells" the board to update — it just reacts.
