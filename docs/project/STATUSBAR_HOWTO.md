# STATUSBAR_HOWTO.md — The status bar

> How the top status bar works: the Zowi face from the original Android app, the
> robot's name and state, and chips for battery, MAC, transport and firmware.
> It was designed in a proposal + proof of concept and then implemented; this
> document records the design decisions.

## Table of contents

- [Files](#files)
- [What it shows](#what-it-shows)
- [States](#states)
- [Chips](#chips)
- [Responsive behaviour](#responsive-behaviour)
- [Detail panel](#detail-panel)
- [Original iconography](#original-iconography)
- [Decisions](#decisions)
- [Previewing and testing](#previewing-and-testing)
- [i18n](#i18n)

## Files

| File | Role |
|---|---|
| `src/views/components/StatusBarView.qml` | Pure presentation. Takes plain properties (`connected`, `connecting`, `demo`, `transportLost`, `hasSaved`, `name`, `mac`, `battery`, `lowBatteryThreshold`, `transport`, `appId`) and draws everything. Knows nothing about `Robot` or `Session`. |
| `src/views/components/StatusBar.qml` | Thin adapter: binds `Robot` and `Session` onto `StatusBarView` and handles *Reconnect*. This is what `ScreenTemplate` and `HomeScreen` instantiate. |
| `src/views/screens/StatusBarPreviewScreen.qml` | Dev preview that drives `StatusBarView` with every state and several widths. |
| `images/android/zowi_{connected,low_battery,altered,disconnected}_icon.png` | The original faces. |

The bar height is the `status_bar_height` key of `config.json` (default **64**,
clamped to 36–96; see [CONFIG_HOWTO.md](CONFIG_HOWTO.md)). `ScreenTemplate` reserves
`statusBar.barHeight` and anchors the rest of the screen under it. Everything inside is
drawn for 48 px and scaled by `height / 48` (face, fonts, label heights, battery icon,
spacing), so the contents follow the height and always spans the full window width. Screens with
`showStatusBar: false` (Scan, Wizard, WizardFound) are unaffected.

## What it shows

- **Face** (32 px): the main indicator of the state.
- **Name** (bold) and **state in words** next to it, so the state never depends
  on colour alone. The name is the registered Zowi's name, or "Zowi" when unknown.
- **Chips** right after the text (see below).
- The whole group (face, name + state, chips) is **centred** in the bar, so the
  pieces stay close together instead of being pushed to both edges.
- The bar background keeps the existing palette (`color_bg_*`).

## States

| Situation | Face | Background | State text |
|---|---|---|---|
| Connected, base firmware | connected (teal, smiling) | `color_bg_connected` | Connected |
| Connected, battery low | low_battery (red, sad) | `color_bg_low_battery` | Low battery! |
| Connected, firmware not the base one | altered (orange, flat mouth) | orange (`color_bg_disconnected`) | Custom firmware |
| Connecting | disconnected + pulsing blue ring | `color_bg_connecting` | Connecting… |
| Disconnected, saved Zowi | disconnected (grey, sad) | neutral grey | Zowi unavailable |
| No registered Zowi | disconnected | neutral grey | Not connected |
| Transport lost | disconnected + red ring | `color_bg_low_battery` | Zowi unavailable (USB/Bluetooth) |
| Demo | green **outline** face, drawn in code (no fill) | none (transparent) | Demo mode |

- *Low battery wins over altered* for the face, as it is the safety-relevant one;
  the firmware chip still shows the difference.
- *Altered* means `Robot.appId` is known and differs from `ZOWI_BASE_v2` (the same
  rule `SettingsScreen` uses for the base firmware).
- Low battery is `battery < low_battery_threshold` (the same rule as
  `Robot.batteryLow`).
- Orange is used **only** for "altered"; disconnected states are grey.
- The demo face is drawn with a `Canvas` (coordinates of the 224 px original), so it
  stays sharp at any size and needs no image.

## Chips

In this order, each shown only when it has data:

1. **Battery** — filled chip: green, or red when low. Icon drawn in code (no
   original bitmap exists) plus the percentage, in white.
2. **Transport** — `BT`: yellow text on blue. `USB`: brown text on yellow.
3. **Firmware** — the `appId`; highlighted in orange with a thicker border when it
   is not the base firmware.
4. **MAC** — blue bold text, blue border. Always visible while known (the saved
   address is kept while disconnected).
5. **Reconnect** — bordered button, only when a Zowi is saved, the robot is not
   connected/connecting and the transport is not lost.

Battery, transport and firmware only appear while connected.

## Responsive behaviour

The group is centred horizontally and its parts are vertically centred. When the
whole group does not fit in the window width (minus a 12 px margin on each side),
chips are kept by priority — Reconnect, battery, MAC, transport, firmware — and the first one that
does not fit hides itself **and every lower-priority one** (so no lonely chip is
left). In the narrowest case only the face, the name and the state remain.

## Detail panel

A click anywhere on the bar opens an **in-window popup** (not a window-type popup,
because of Wayland), placed right under the bar and **centred horizontally** on it with the state, MAC, transport, firmware (marking whether it is
the base one) and battery with its warning threshold. If *Reconnect* does not fit in
the bar, the button is shown inside this panel so the action is never lost.

## Original iconography

The Android app had five round Zowi faces (kept in `docs/images/android/`):

| Original state | File | Used for |
|---|---|---|
| Connected | `zowi_connected_icon.png` | Connected |
| Low battery | `zowi_low_battery_icon.png` | Battery below the threshold |
| Altered | `zowi_altered_icon.png` | Non-base firmware |
| Disconnected | `zowi_disconnected_icon.png` | Disconnected, connecting, transport lost, not registered |
| Demo | `zowi_demo_icon.png` | **Not used**: the demo face is the green outline drawn in code |

The shipped PNGs in `images/android/` are RGBA (the originals were palette PNGs and
looked noisy when scaled down to 32 px). There is no original icon for battery,
Bluetooth, USB or firmware.

## Decisions

1. **Altered (orange)** = connected with a firmware that is not the base one.
2. **MAC** is always visible in the bar (not hidden behind the detail panel).
3. **Demo** has no background and an outline face in green, instead of grey/red.
4. **Disconnected** uses a neutral grey background so orange only means "altered".
5. **Chip colours:** BT yellow-on-blue, USB brown-on-yellow, MAC blue bold,
   battery green / red.
6. **Centred group:** face, text and chips are centred together (not split left/right).
7. **Narrow windows:** chips hide from the right until only face, name and state
   are left.
8. **Full width:** the bar background always stretches across the whole window.

## Previewing and testing

```bash
./src/views/tests/preview-statusbar.sh
```

`StatusBarPreviewScreen` has one button per state, width presets (*Auto* follows the
real window, so it can be resized freely) and a battery slider. Clicking the bar
opens the detail panel. For headless captures `--step N` selects the state with
`N % 100` and the simulated width with `N / 100`; see `src/views/tests/README.md`.

The preview app's `Robot`/`Session` mocks (`src/gui/preview_main.cpp`) expose the
subset the bar needs (`situation`, `battery`, `appId`, `activeTransport`,
`loadActiveZowiTransport`…), so every screen that has a status bar previews without
QML errors.

## i18n

Context `StatusBar.qml` (kept for both QML files). Keys: `connected`,
`status_connecting`, `status_unavailable`, `not_connected`, `status_low_battery`,
`status_demo` ("Demo mode"), `status_custom_firmware`, `status_transport_lost`,
`status_reconnect`, `via_bluetooth`, `via_usb`, `detail_mac`, `detail_transport`,
`detail_firmware`, `detail_firmware_base`, `detail_firmware_custom`,
`detail_battery`, `detail_battery_value`. The `BT`/`USB` chip labels, the firmware
`appId` and the MAC are not translated. The five locale files carry the same keys.
