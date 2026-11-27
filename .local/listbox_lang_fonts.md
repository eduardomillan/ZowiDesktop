# Selector de idioma (SplashScreen): causas y correcciones

**Estado: resuelto** (commits `b940a7f` y `5682d44`, 27-nov-2026). Pendiente solo de
publicar en la 0.9.7.

## Síntoma
En LliureX 25 (Ubuntu Noble, Plasma 6.3.5 en Wayland, Qt 6.9.0 de KDE neon) el
selector de idioma de `src/views/screens/SplashScreen.qml`:
- mostraba en negro solo el idioma seleccionado;
- al desplegar, el resto de idiomas no se veía;
- al navegar con el teclado, cada fila aparecía (en negro) al quedar resaltada.

No ocurría en LliureX 23 (Jammy) ni en el entorno de desarrollo (Qt 6.5.2, X11).

## Causas

### 1. Hipótesis inicial (descartada como causa principal): fuente `monospace`
El selector usaba `font.family: "monospace"`. En LliureX la monospace es Hack, que
puede no cubrir bien acentos o cirílico. Fue un problema real pero secundario: no
explica que solo se viera una fila.

### 2. Causa real: el `ListView` del desplegable no maqueta sus filas en Qt 6.9
El desplegable usaba el patrón habitual (`popup.contentItem: ListView` con
`model: langCombo.delegateModel`). Con Qt 6.9 las cinco filas se crean pero **todas
quedan en `y=0`**, una encima de otra. `ListView` pone la fila actual con `z=1`, así
que solo se ve esa (la resaltada). Por eso el teclado "descubría" cada idioma.

Se descartó, probando variantes con un QML mínimo ejecutado con `qml6` en la propia
máquina: la paleta de Plasma, el recorte (`clip`), el renderer (`QT_QUICK_BACKEND=software`),
la altura fija del popup, `required property` en el delegado, `z` en las filas, enlazar
el modelo solo al abrir, y `forceLayout()` al abrir. Solo funcionó sustituir el
`ListView`.

### 3. Por qué las pruebas en LliureX "seguían igual": caché de QML obsoleta
Se instalaron varias compilaciones con el **mismo número de versión (0.9.6)**. Los
binarios del paquete llevan siempre la misma fecha (la del changelog, por la
compilación reproducible) y Qt valida la caché `~/.cache/ZowiDesktop/ZowiDesktop/qmlcache`
por esa fecha, así que siguió ejecutando el QML antiguo. Con una caché vacía se vio el
QML nuevo. Con una versión nueva (0.9.7) la caché se invalida sola.

Para probar compilaciones de la misma versión: `rm -rf ~/.cache/ZowiDesktop`
(o `XDG_CACHE_HOME=/tmp/cache-limpia ZowiDesktop`).

### 4. Aparte: arranque de ~25 s sin ventana sin Bluetooth
No tiene que ver con el selector, pero salió en la misma sesión. El constructor de
`QtBluetoothBackend` creaba un `QBluetoothLocalDevice`, que consulta BlueZ por D-Bus de
forma síncrona; sin BlueZ en marcha esperaba al timeout de D-Bus (~25 s) en el hilo de
la GUI, antes de crear la ventana.

## Correcciones
- **Selector** (`SplashScreen.qml`): el `ListView` se sustituye por `Column` +
  `Repeater` dentro del `Popup`. Hover, clic y teclado (↑ ↓ Intro Espacio) se
  implementan a mano (`navIndex`, `commit()`). Se mantienen la apertura hacia arriba y
  el borde. Las paletas del `ComboBox` y del `Popup` se fijan con los colores de la app
  como precaución (no dependen del tema de Plasma).
- **Fuente**: Noto Sans (OFL 1.1), `NotoSans-Regular.ttf` y `NotoSans-Bold.ttf` en
  `fonts/` con `OFL.txt`. `fonts.qrc` se genera con `zowi_generate_qrc` en
  `src/gui/CMakeLists.txt`. Se carga en `src/gui/main.cpp` y `src/gui/preview_main.cpp`
  con `QFontDatabase::addApplicationFont` y `app.setFont("Noto Sans")`.
  Se quitó `font.family: "monospace"` de `SplashScreen`, `WelcomeScreen`, `ScanScreen` y
  `ScreenTemplate`; se mantiene en `DevOverlay` y en el patrón de bits de
  `MouthEditorScreen`.
- **Bluetooth** (`qt_bluetooth_backend.{h,cpp}`): `QBluetoothLocalDevice` pasa a ser un
  `std::unique_ptr` creado bajo demanda en `localDevice()`, solo al hacer `unpair()`.
- **CHANGELOG**: dos entradas en *Fixed* bajo `[Unreleased]`.

## Verificación
- Con una caché vacía, en LliureX 25 (Qt 6.9.0, Wayland) se vieron las cinco filas con
  su resaltado, y elegir "Français" por código cambió el selector.
- No se probaron en esa máquina las teclas ni el ratón reales (sin forma de inyectarlos
  por `ssh`): conviene una prueba manual.
- No se reprodujo el caso sin Bluetooth (el adaptador de la máquina estaba activo).

## Cómo reproducir problemas parecidos sin tocar el escritorio
`qml6` está instalado en LliureX 25. Un QML mínimo con un `Timer` que llame a
`langCombo.popup.open()` y una captura con `spectacle -b -n -f` evita necesitar clics.
Ojo: `spectacle -a` captura la ventana activa, que puede ser ajena a la app, y
`pkill -f` desde un `ssh` coincide con su propia línea de comandos.

## Archivos
- `src/views/screens/SplashScreen.qml`
- `src/gui/main.cpp`, `src/gui/preview_main.cpp`, `src/gui/CMakeLists.txt`
- `fonts/` (`NotoSans-Regular.ttf`, `NotoSans-Bold.ttf`, `OFL.txt`)
- `src/backends/bt_qt/qt_bluetooth_backend.{h,cpp}`
