# Plan de ranking para ZowiDesktop

> Documento de planificación (histórico). El funcionamiento real y actualizado está en `docs/project/RANKING_HOWTO.md`.


## Context

El ranking no existe todavía en ZowiDesktop. Hay botones de ranking deshabilitados (`GameTimelineScreen.qml`, `GameMouthsScreen.qml`, `GameMemoryScreen.qml`), umbrales sin usar (`rankThreshold=3` en `zowi_dice.h`, `rankScoreThreshold=2` en `mouths_game.h`), y solo se guarda la última puntuación (`zowi_says_last_score`, `mouths_last_score`). El Android original (`~/ZowiAppReborn`) tiene ranking local top-10 solo para Zowi Says y Mouths, sin backend alguno. Se quiere: (1) completar la lógica local, (2) definir un ranking para juegos que no lo tenían (Timeline), (3) una opción online usando la infraestructura GitHub existente.

## Hallazgos clave

**Android original** (`AndroidCoreRankingController.kt`, `MakerBoxDialogRanking.java`, presenters de Zowi Says/Mouths):
- Clave prefs `<GAME_ID>_ranking` = JSON `[{points, playerName, timestamp}]`, ordenado desc, máximo 10.
- `isScoreInTop10`: true si hay <10 entradas o score > última.
- Flujo: al terminar, puntos = `size-1` (Says) o `level-1` (Mouths); entra al ranking solo si `size>3` / `level>2` **y** está en top 10; entonces pide nombre. Si no, diálogo "no has entrado en el ranking".
- UI: diálogo overlay (no pantalla), filas con icono de posición, nombre y puntos, resalta la última entrada. Vacío: texto de lista vacía.
- Reset al "olvidar historial". Timeline y Gamepad no tienen ranking (solo logros).

**ZowiDesktop**: sin `RankingController`, sin `RankingScreen`, sin logros. `SessionController` solo expone `saveString/getString` a QML (listas = JSON en string). Assets Android ya presentes: `ranking_button.png`, `pressed_ranking_button.png`, `ranking_icon.png`, `ranking_image.png`. i18n son JSON (`i18n/zowi_{en_US,es_ES,ca_ES,fr_FR,bg_BG}.json`), faltan claves de ranking. `new_best` está hardcodeado a `score>=12`.

**Infra online**: GitHub Pages (`gh-pages`, `docs/`) es estático y de solo lectura; no hay código de red (`Qt6::Network` ausente), ni workflows de Pages/cron, ni secretos. Una PAT en el cliente sería extraíble (ver `.local/GITHUB_TOKEN_SECURITY.md`). App infantil: privacidad/COPPA/GDPR → opt-in, sin datos personales.

## Diseño propuesto

### Capa 1 — Ranking local (core, Qt-free)

Nuevo `src/core/include/zowi/ranking_store.h` + `src/core/src/ranking_store.cpp`, con su propio archivo `ZowiRanking.json` (un `SessionStore` genérico distinto del de la sesión):
- `struct RankingEntry { int points; std::string playerName; int64_t timestamp; }`.
- `enum class GameId { ZowiSays, Mouths, Timeline }` → clave `"<game>"` dentro de `ZowiRanking.json` (JSON array, mismo formato que Android).
- API: `top(gameId)`, `isInTopN(gameId, score)` (N=10), `submit(gameId, name, score)` (inserta, ordena desc, recorta a 10, devuelve posición), `best(gameId)`, `clear(gameId)`, `clearAll()`.
- Reutiliza el (de)serializado JSON ya usado en `timeline_command.cpp` (nlohmann_json).
- Reglas de elegibilidad centralizadas (reutilizan los umbrales existentes): Says `size>3`, Mouths `level>2`, Timeline definido abajo.
- Tests en `src/core/tests/test_ranking_store.cpp` (orden, recorte a 10, empates, top10, clear).

### Capa 2 — Controller + UI (GUI)

- `src/gui/controllers/RankingController.{h,cpp}`: Q_INVOKABLE `top(game)` → QVariantList, `qualifies(game, score)`, `submit(game, name, score)`, `best(game)`; señal `rankingChanged`. **Sin borrado desde QML** (ver política de admin). Se registra en `main.cpp` junto a los demás (una sola vez, ojo con el duplicado de wiring visto en Timeline).
- `src/views/components/RankingDialog.qml` (diálogo overlay, como Android): título, `ListView` con fila (icono posición + nombre + puntos), fila de la última entrada resaltada, estado vacío. Reutiliza `ranking_icon.png`.
- Activar el botón ranking (hoy `enabled:false`) en los 3 juegos; mostrar el diálogo.
- Flujo fin de partida: si `qualifies` → pedir apodo (diálogo con campo de texto) → `submit` → abrir ranking resaltando la entrada; si no → mensaje "no has entrado".
- Sustituir `new_best` hardcodeado por `score > RankingController.best(game)`.
- i18n: añadir claves (`ranking_title`, `ranking_empty`, `ranking_points` plural, `ranking_enter_name`, `ranking_not_qualified`, `ranking_share_online`, etc.) en los 5 JSON de locale, contexto por pantalla.
- Settings: "Borrar logros" (`SettingsScreen.qml:158`, hoy stub) borrará **solo los logros**, nunca el ranking. Mientras no exista la capa de logros se mantiene el texto de relleno, corregido en los 5 idiomas (`achievements_stub`: "los rankings no se ven afectados").
- Ranking de Zowi Says/Mouths: score = `size-1` / `level-1` (como Android) vía `ZowiDiceController::gameOver` y `MouthsGameController` (puntos ya existentes, `ZowiDiceController.cpp:118`, `MouthsGameController.cpp:107`).

### Capa 3 — Ranking para Timeline (nuevo respecto a Android)

Decisión del usuario: **complejidad ponderada**. Propuesta de fórmula (constantes en un `TimelineScoreConfig` en core, ajustables y con test):
- Por step: movimiento = 3 pts × repeticiones; gesto/animación = 2 pts × repeticiones; boca = 1 pt.
- Multiplicador de velocidad opcional en movimientos (Fast ×1.2 / Medium ×1.0 / Slow ×0.8) — decidir al implementar si se mantiene.
- Bonificación por variedad: +2 por cada tipo distinto de acción (movimiento/gesto/boca) usado (máx. +6).
- Anti-trampa: la puntuación solo se otorga **al completar una reproducción entera** con acks reales (`TimelinePlayer` llega a `Finished`), y se calcula desde los `TimelineStep` realmente enviados. Tope de 40 steps en el cálculo.
- Elegibilidad: ≥ 5 steps (análogo a los umbrales de los otros juegos).
- Cálculo en core: `src/core/include/zowi/timeline_score.h` (puro, testeable). `TimelineController::sendNextCommand()` (rama `finished()`) emite una señal `sequenceCompleted(score)`; `GameTimelineScreen.qml` ofrece guardar en ranking.
- Se mantiene el logro `anxious` (≥15 comandos) como futuro de la capa de logros (fuera de alcance).

### Capa 4 — Ranking online (opcional, opt-in)

**Respuesta a "¿basta con GitHub?": no del todo.** GitHub Pages sirve para *leer* (JSON estático) pero no para *escribir*; guardar una PAT en el cliente (open source, binarios distribuidos) la filtraría. Se necesita **un componente de escritura mínimo**. Decisión del usuario: proxy serverless.

Arquitectura:
```
App ──POST {game, nick, score, installTs, version, hmac?}──▶ Cloudflare Worker (gratis)
        │                                   │ valida (caps, rate-limit, filtro apodo)
        │                                   ▼ commit vía GitHub API (token solo en el Worker)
        │                              repo/gh-pages: docs/ranking/<game>.json
        └──GET https://eduardomillan.github.io/ZowiDesktop/docs/ranking/<game>.json
```
- Lectura: `QNetworkAccessManager` → JSON estático de Pages (cacheable, sin servidor propio). Si falla la red, se usa el ranking local (offline-first).
- Escritura: Worker con KV para rate-limit por IP/instalación; valida: juego conocido, score ≤ máximo plausible por juego (Says ≤ ~60, Mouths ≤ ~40, Timeline ≤ tope de la fórmula), apodo ≤ 12 chars con filtro de palabras, un envío cada N segundos. Mantiene top-100 por juego y actualiza el JSON en `gh-pages` (solo bajo `docs/ranking/`, sin tocar `docs/dists`/`pool`, como exige AGENTS.md).
- Identidad: **apodo libre opt-in, sin cuentas**, sin datos personales ni nombre del Zowi. Checkbox explícito "compartir online" por envío (por defecto desactivado) + aviso de privacidad (app infantil, COPPA/GDPR).
- Limitación honesta: las puntuaciones son reportadas por el cliente → trampeable. Mitigación: topes plausibles, rate-limit, y moderación manual (borrar entradas del JSON en `gh-pages`); un HMAC en el cliente no es seguro (open source) y no se usa como garantía.
- Cliente: nueva clase `src/gui/controllers/OnlineRankingClient.{h,cpp}` (Qt6::Network, añadir a `find_package` en CMake). `src/core` sigue Qt-free (solo formato/validación JSON compartida). Config en `src/config.json` (estilo existente, strings): `ranking_online_enabled:"false"`, `ranking_online_read_url`, `ranking_online_submit_url`. Pestaña "Local / Mundial" en `RankingDialog.qml`.
- Infra nueva a crear (fuera del repo de la app): Cloudflare Worker + secreto con token de GitHub de alcance mínimo (fine-grained, solo `contents:write` sobre este repo). Documentar en `docs/project/RANKING_HOWTO.md`; sin tocar `release.yml`.

## Política de borrado (decidida por el usuario)

- Ningún usuario puede borrar el ranking desde la app: ni "Borrar logros" ni ningún otro botón.
- **Local:** solo un admin, mediante la CLI: `zowi_cli ranking list [juego]` y `zowi_cli ranking clear [juego|all]` (juegos: `zowi_says`, `mouths`, `timeline`). `RankingStore::clear/clearAll` queda en el core solo para ese uso. El ranking vive en su **propio archivo** (`ZowiRanking.json`), separado de la sesión (`ZowiApp.json`): `zowi_cli session clear` no lo toca.
- **Online:** solo el mantenedor, editando a mano el JSON de `docs/ranking/` en `gh-pages` (sin endpoint de borrado en el cliente ni en el Worker).

## Fases y orden de implementación

1. **Core local**: `ranking_store` + tests; `timeline_score` + tests.
2. **GUI local**: `RankingController`, `RankingDialog.qml`, activar botones, flujo de apodo, i18n ×5, reset desde Settings, `new_best` real. Integrar Zowi Says y Mouths.
3. **Timeline**: señal `sequenceCompleted`, puntuación y ranking.
4. **Online lectura** (Worker aún no necesario): cliente + ranking mundial de solo lectura publicado a mano/Action.
5. **Online escritura**: Worker, validación, opt-in y privacidad, pestaña "Mundial".
6. **Docs**: `RANKING_HOWTO.md`, actualizar `PLANNING.md` (casillas "Ranking in games") y los `SCREEN_GAME_*.md`.

## Archivos críticos

Nuevos: `src/core/{include/zowi,src}/ranking_store.*`, `timeline_score.*`, `src/gui/controllers/RankingController.*`, `OnlineRankingClient.*`, `src/views/components/RankingDialog.qml`, tests en `src/core/tests/`.
Modificados: `src/gui/main.cpp`, `src/gui/CMakeLists.txt`/`CMakeLists.txt`, `ZowiDiceController.cpp`, `MouthsGameController.cpp`, `TimelineController.{h,cpp}`, `GameMemoryScreen.qml`, `GameMouthsScreen.qml`, `GameTimelineScreen.qml`, `SettingsScreen.qml`, `i18n/zowi_*.json`, `src/config.json`.
Reutilizar: `SessionStore` (`session_store.h`), serialización JSON de `timeline_command.cpp`, umbrales de `zowi_dice.h`/`mouths_game.h`, assets `ranking_*.png`.

## Riesgos / decisiones abiertas

- Fórmula de Timeline: valores iniciales a validar jugando; mantenerla en config para ajustarla sin tocar lógica.
- Moderación de apodos y menores: considerar desactivar el envío por defecto y mostrar aviso a padres.
- Dependencia de un servicio externo (Cloudflare): el ranking online debe degradar con elegancia a local.
- `Session` solo expone strings a QML: usar JSON string o ampliar `SessionController` con `getInt/setInt`.

## Verificación

- Core: `cmake --build build --target test_ranking_store test_timeline_score && ctest --test-dir build -R 'ranking|timeline_score' --output-on-failure`.
- GUI: `./build.sh --gui`; jugar Zowi Says/Mouths/Timeline, comprobar apodo, orden, límite 10, resaltado de la última entrada, estado vacío y borrado desde Settings (con y sin robot en modo demo).
- Online: Worker en local (`wrangler dev`) contra un repo de pruebas; probar caps, rate-limit, apodo inválido, red caída (fallback local) y que el JSON de `gh-pages` solo cambia bajo `docs/ranking/`.
