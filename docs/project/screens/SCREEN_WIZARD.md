# SCREEN_WIZARD — WizardScreen.qml

> Bluetooth pairing wizard entry: prompts to turn the Zowi on first, with
> "already on" vs "no Zowi" choices. Also handles the edge cases where only USB
> is available (skips the Bluetooth scan) or no transport is available at all.

- **File:** `src/views/screens/WizardScreen.qml`
- **i18n context:** `"WizardScreen.qml"`
- Pushed by `WelcomeScreen.startWizard` (via `main.qml`).

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `startClicked()` | "Already on" button (also fired by `usbWarnTimer`) | `main.qml`: push→ Scan (BT) or WizardFound with `usbMode` (USB-only) |
| `dismissed()` | "No Zowi / skip" | `main.qml`: `Session.saveWizardDismissed(true)` → replace→ Home |

## QML context used

- `Robot`: `bluetoothAvailable`, `usbAvailable`, `usbZowiConfirmed`.
- `Config.get(...)`: `welcome_image` and theme colors.
- `MessageBar` in the footer for transport warnings.

## Commands sent

- None.

## Implementation notes

- `usbWarnTimer` (3 s): when a USB robot is confirmed while Bluetooth is also
  available, shows the "usb_recommend_disconnect" warning, then fires
  `startClicked()`.
- `noTransportTimer` (3 s): when neither transport exists, shows
  `no_transport_error` in the footer and auto-dismisses the wizard.
- `showStatusBar: false`; footer is a `MessageBar`.