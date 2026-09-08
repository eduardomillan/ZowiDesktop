# SCREEN_CALIBRATION — CalibrationScreen.qml

> Interactive servo-trim calibration: a 4-step pager (WARNING → LEGS → FEET →
> CHECK) mirroring the Zowi Android app's `CalibrationViewActivity`. All state
> lives in the C++ `CalibrationSession` core module, exposed to QML as
> `Calibration`.

- **File:** `src/views/screens/CalibrationScreen.qml`
- **i18n context:** `"CalibrationScreen.qml"`
- Pushed by `SettingsScreen.calibrationRequested` (and, since v0.7.4, by the
  Gravity project's action button — `action_target: "calibration"` in
  `projects/gravity/project.json`). Shows the Gravity project detail image
  (`qrc:/images/projects/gravity_thumb.png`) below the title when opened from a
  project, with its width parametrizable via `projectImageWidth`.

## Signals

- None declared; navigation uses `ScreenTemplate`'s `backClicked`.

## QML context used

- `Calibration` (`CalibrationSessionController`): `adjust(index, delta)`,
  `sendServos()`, `resetToNeutral()`, `trimsCommand()`, `jumpToStep(s)`,
  `step`, `trimYL`/`trimYR`/`trimRL`/`trimRR`.
- `Robot`: `connected`, `sendData(cmd)`, `setDataPollingEnabled(false/true)`.
- `Config.get(...)`: theme colors.
- Optional `PreviewStep` variable (from `zowi_screen_preview --step N`).

## Commands sent

| UI action | Builder | Wire command |
|-----------|---------|--------------|
| Live trim move | `Calibration.sendServos()` | `G <yl> <yr> <rl> <rr>\r` (core keeps one `G` in flight) |
| Neutral / home | `Calibration.resetToNeutral()` | servo stop (`S`), servos → 90° + detach |
| Save trims | `Calibration.trimsCommand()` | `C <yl> <yr> <rl> <rr>\r` (persists to EEPROM) |
| After save | literal | `H 12\r` (VICTORY gesture) |

## Implementation notes

- `sendTimer` (200 ms) re-flushes the servos when the core debounce coalesces a
  quick adjustment (mirrors the CLI `needSend` loop).
- `Component.onCompleted`: fresh `Calibration.reset()` (or
  `jumpToStep(PreviewStep)`), disables data polling
  (`setDataPollingEnabled(false)`) to keep the channel clean; re-enabled on
  destruction.
- The trim columns reuse the `CalibrationTrimColumn` component
  (`src/views/components/CalibrationTrimColumn.qml`: ±1 fine, ±10 coarse).
- Trims are clamped to **±60°** by the core `CalibrationSession`.
- CLI counterpart: `zowi_cli calibrate` (interactive wizard or
  `--yl/--yr/--rl/--rr` direct mode).