# PadScreen — Plan de implementación

Estado actual de la PadScreen y lo que falta para equipararla a la Android original.

## Lo que ya funciona

- **movementPad** (izquierda): direcciónes, turnos, moonwalkers — todo cableado vía `startHold()`/`stopHold()` con `Robot.sendData()`
- **actionPad** (derecha): bend, shake leg, updown, jitter, swing, flapping, crusaito — todo funcional
- **speedControl** (centro abajo): cicla slow→medium→fast al hacer clic, imágenes superpuestas con `visible` toggle

## Lo que está mal / falta

### 1. Botón de bocas (`mouthsBtn`)
- **Estado:** Stub — solo imprime `"not yet implemented"` al hacer clic
- **Necesita:** Abrir un `MouthsDialog` con grid de 20 bocas

### 2. Botón de animaciones/gestures
- **Estado:** **No existe** en la UI
- **Necesita:** Crear botón + `AnimationsDialog` con grid de 11 gestos

### 3. Diálogo de bocas
- **Estado:** No existe ningún QML
- **Necesita:** `MouthsDialog.qml` — grid con las 20 bocas, cada una envía `L <30bits>\r`
- **Assets disponibles en `images.qrc`:**
  - `smile_button.png` / `smile_icon.png`
  - `happy_open_button.png` / `happy_open_icon.png`
  - `sad_button.png` / `sad_icon.png`
  - `sad_open_button.png` / `sad_open_icon.png`
  - `sad_closed_button.png` / `sad_closed_icon.png`
  - `tongue_out_button.png` / `tongue_out_icon.png`
  - `angry_button.png` / `angry_icon.png`
  - `confused_button.png` / `confused_icon.png`
  - `ok_mouth_button.png` / `ok_mouth_icon.png`
  - `x_mouth_button.png` / `x_mouth_icon.png`
  - `line_mouth_button.png` / `line_mouth_icon.png`

### 4. Diálogo de animaciones
- **Estado:** No existe ningún QML
- **Necesita:** `AnimationsDialog.qml` — grid con 11 gestos, cada uno envía `H <id>\r`
- **Assets disponibles:**
  - `animation_happy_button.png`
  - `animation_super_happy_button.png`
  - `animation_sad_button.png`
  - `animation_sleppy_button.png`
  - `animation_fart_button.png`
  - `animation_confused_button.png`
  - `animation_in_love_button.png`
  - `animation_angry_button.png`
  - `animation_anxious_button.png`
  - `animation_magic_button.png`
  - `animation_wave_button.png`

### 5. Comandos C++ que no existen
- `commandGesture(int gestureId)` → `H <id>\r` (wire ID 1-13)
- `commandMouth(const std::string &binary)` → `L <30bits>\r`

Ambos deben añadirse a `robot_commands.h` / `robot_commands.cpp`.

## Datos de protocolo (desde zowiLibs)

### Gesture IDs (comando `H <id>`)
Wire ID 1-13, mapeo en firmware `receiveGesture()`:

| Wire ID | Nombre | Constante interna |
|---------|--------|-------------------|
| 1 | Happy | `ZowiHappy` (0) |
| 2 | Super Happy | `ZowiSuperHappy` (1) |
| 3 | Sad | `ZowiSad` (2) |
| 4 | Sleeping | `ZowiSleeping` (3) |
| 5 | Fart | `ZowiFart` (4) |
| 6 | Confused | `ZowiConfused` (5) |
| 7 | Love | `ZowiLove` (6) |
| 8 | Angry | `ZowiAngry` (7) |
| 9 | Fretful | `ZowiFretful` (8) |
| 10 | Magic | `ZowiMagic` (9) |
| 11 | Wave | `ZowiWave` (10) |
| 12 | Victory | `ZowiVictory` (11) |
| 13 | Fail | `ZowiFail` (12) |

### Mouth/LED binary patterns (comando `L <30bits>`)
El firmware llama `strtoul(arg, &endstr, 2)` para convertir el string binario a uint32, luego `zowi.putMouth(matrix, false)`. Matriz 5×6 LEDs.

Patrones disponibles (`Zowi_mouths.h`):

| Índice | Nombre | Binario (30 bits) |
|--------|--------|-------------------|
| 0 | zero | `00001100010010010010010010001100` |
| 1 | one | `00000100001100000100000100001110` |
| 2 | two | `00001100010010000100001000011110` |
| 3 | three | `00001100010010000100010010001100` |
| 4 | four | `00010010010010011110000010000010` |
| 5 | five | `00011110010000011100000010011100` |
| 6 | six | `00000100001000011100010010001100` |
| 7 | seven | `00011110000010000100001000010000` |
| 8 | eight | `00001100010010001100010010001100` |
| 9 | nine | `00001100010010001110000010001110` |
| 10 | smile | `00000000100001010010001100000000` |
| 11 | happyOpen | `00000000111111010010001100000000` |
| 12 | happyClosed | `00000000111111011110000000000000` |
| 13 | heart | `00010010101101100001010010001100` |
| 14 | bigSurprise | `00001100010010100001010010001100` |
| 15 | smallSurprise | `00000000000000001100001100000000` |
| 16 | tongueOut | `00111111001001001001000110000000` |
| 17 | vamp1 | `00111111101101101101010010000000` |
| 18 | vamp2 | `00111111101101010010000000000000` |
| 19 | lineMouth | `00000000000000111111000000000000` |
| 20 | confused | `00000000001000010101100010000000` |
| 21 | diagonal | `00100000010000001000000100000010` |
| 22 | sad | `00000000001100010010100001000000` |
| 23 | sadOpen | `00000000001100010010111111000000` |
| 24 | sadClosed | `00000000001100011110110011000000` |
| 25 | okMouth | `00000001000010010100001000000000` |
| 26 | xMouth | `00100001010010001100010010100001` |
| 27 | interrogation | `00001100010010000100000100000100` |
| 28 | thunder | `00000100001000011100001000010000` |
| 29 | culito | `00000000100001101101010010000000` |
| 30 | angry | `00000000011110100001100001000000` |

### Layout objetivo (Android original)
Los 3 botones centrales alineados verticalmente entre movementPad y actionPad:

```
[movementPad]    [speed]   [actionPad]
                 [mouths]
                 [anims]
```

## Orden de implementación propuesto

1. [ ] Añadir `commandGesture()` y `commandMouth()` en `robot_commands.h/cpp`
2. [ ] Crear `MouthsDialog.qml` en `src/views/components/`
3. [ ] Crear `AnimationsDialog.qml` en `src/views/components/`
4. [X] Añadir botón de animaciones a PadScreen
5. [ ] Cablear `mouthsBtn` para abrir `MouthsDialog`
6. [ ] Cablear botón de animaciones para abrir `AnimationsDialog`
7. [ ] Ajustar layout de los 3 botones centrales
8. [ ] Verificar build y tests
