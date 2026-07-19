# Plan: Arreglar la visualización de idiomas en el selector (ListBox)

## Contexto
En Lliurex 25 (Ubuntu Noble) el ComboBox de selección de idioma del SplashScreen
(`src/views/screens/SplashScreen.qml`) muestra "items en blanco/cajas" para varios
idiomas. El modelo tiene los 5 idiomas correctamente (ListModel estático, no hay
filtrado). El problema es de **renderizado de glifos**, no de lógica.

Causa raíz: `SplashScreen.qml:164` usa `font.family: "monospace"` (fuente del
sistema). En Lliurex 25 la monospace del sistema no cubre acentos (Valencià,
Français) ni cirílico (Български), así que esos textos se renderizan en blanco.

Locales soportados (hardcodeados): es_ES, ca_ES, en_US, fr_FR, bg_BG.

## Solución propuesta
Empaquetar una fuente con cobertura Unicode amplia (Noto Sans / DejaVu Sans, licencia
OFL) y usarla en el selector (y de paso en toda la app, ya que otras pantallas también
muestran textos traducidos con esos glifos).

### Pasos
1. **Añadir fuente al repo**
   - Crear `fonts/NotoSans-Regular.ttf` (u otra OFL) con cobertura latina + cirílica.
   - Registrarla en un nuevo `fonts.qrc` (o dentro de `app.qrc`).

2. **Cargar la fuente**
   - Opción A (mínima): `FontLoader { id: appFont; source: "qrc:/fonts/..." }` en
     `main.qml` / `SplashScreen.qml` y usar su `name` en el ComboBox.
   - Opción B (global, recomendada): en `src/gui/main.cpp` registrar con
     `QFontDatabase::addApplicationFont` y `app.setFont(...)` para que TODAS las
     traducciones rendericen bien en toda la app, no solo el Splash.

3. **Usar la fuente en el ComboBox**
   - Cambiar `font.family: "monospace"` (línea 164) y los `contentItem`/`delegate`
     (líneas 190, 212) para usar la familia cargada.
   - Evaluar migrar también ScreenTemplate, WelcomeScreen, ScanScreen, DevOverlay
     (todos usan monospace) para cobertura consistente de traducciones.

4. **Build y verificar**
   - `./build.sh --gui`
   - Comprobar visualmente que los 5 idiomas se ven en el selector.
   - `ctest --test-dir build --output-on-failure` para no romper tests core.

## Decisiones pendientes
- ¿Fuente solo para el selector (A) o para toda la app (B)?
- ¿Noto Sans (OFL) está bien, o hay una fuente preferida ya presente en Lliurex?

## Archivos clave
- `src/views/screens/SplashScreen.qml` (líneas 154-164, 185-242)
- `src/gui/main.cpp` (línea 48 estilo "Basic", 85-89 carga QML)
- `src/gui/preview_main.cpp` (línea 203)
- `app.qrc` / `views.qrc` / `images.qrc` / `i18n.qrc` (ninguno tiene fuentes hoy)
