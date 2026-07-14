# Relación con zowiLibs

[zowiLibs](https://github.com/eduardomillan/zowiLibs) es el proyecto **hermano** de
ZowiDesktop: contiene el firmware que se ejecuta en el robot Zowi (versión original de
BQ, adaptada por el mismo autor) y las librerías Arduino que lo acompañan. Ambos
proyectos se mantienen en repositorios GitHub independientes.

## ¿Qué aporta zowiLibs a ZowiDesktop?

| Componente | En zowiLibs | En ZowiDesktop | Relación |
|---|---|---|---|
| Firmware `ZOWI_BASE_v2.hex` | `code .hex/ZOWI_BASE_v2.hex` | `src/firmware/ZOWI_BASE_v2.hex` | Byte-idéntico (solo difiere CRLF→LF). Se copia y normaliza mediante el script `scripts/sync_firmware_from_zowiLibs.sh`. |
| Firmware `ZOWI_Alarm_v2.hex` | compilado desde `code .ino/games/ZOWI_Alarm_v2/` | `src/firmware/ZOWI_Alarm_v2.hex` | Derivado de la fuente de zowiLibs. |
| Protocolo de comunicación | `ZowiSerialCommand` ( `&&`/`%%` ) y comandos `S/L/T/M/H/K/C/G/R/E/D/N/B/I/A/F` | `src/core/include/zowi/protocol.h` | El header `protocol.h` define las constantes (**única fuente de verdad**) y `makeCommand()` para construir comandos host. |
| Librerías Arduino (Zowi, Oscillator, LedMatrix, US, BatReader, EnableInterrupt) | `arduino libraries/` | — | **No se reutilizan.** Son específicas de AVR/Arduino y corren en el robot, no en la aplicación de escritorio. |

## Las librerías Arduino del robot (no reutilizables)

Las librerías de la carpeta `arduino libraries/` implementan el comportamiento físico
del robot: control de servos (Oscillator, Zowi), matriz LED (LedMatrix), sensor de
ultrasonidos (US), lectura de batería (BatReader) e interrupciones externas
(EnableInterrupt). Están escritas para el ecosistema Arduino/AVR y dependen de
hardware (`<Servo.h>`, `EEPROM`, `analogRead`, `digitalWrite`, etc.) que no existe
en una aplicación de escritorio.

ZowiDesktop modela el robot a través de su propia capa de abstracción:
`DeviceInfo`, `BluetoothApi` y `robot_commands.h`. No intenta compilar el código
AVR nativo.

## Tabla del protocolo

| Dirección | Comando | Significado | Respuesta |
|---|---|---|---|
| → robot | `S` | Stop / home | `&&A%%` `&&F%%` |
| → robot | `L <leds>` | Control matriz LED | `&&A%%` `&&F%%` |
| → robot | `T <note>` | Zumbador | `&&A%%` `&&F%%` |
| → robot | `M <id> <T>` | Movimiento (1 walk, 2 backward, 3 turnL, 4 turnR) | `&&A%%` `&&F%%` |
| → robot | `H <id>` | Gesto | `&&A%%` `&&F%%` |
| → robot | `K <melody>` | Canción | `&&A%%` `&&F%%` |
| → robot | `C <trims>` | Ajuste servos | `&&A%%` `&&F%%` |
| → robot | `G <servo> <angle>` | Servo directo | `&&A%%` `&&F%%` |
| → robot | `R <nombre>` | Renombrar (escribe EEPROM) | `&&A%%` `&&F%%` |
| → robot | `E` | Pedir nombre | `&&E <nombre>%%` |
| → robot | `D` | Pedir distancia | `&&D <cm>%%` |
| → robot | `N` | Pedir ruido | `&&N <valor>%%` |
| → robot | `B` | Pedir batería | `&&B <%>%%` |
| → robot | `I` | Pedir ID de programa | `&&I <id>%%` |
| ← desktop | `&&A%%` | Ack (comando recibido) | — |
| ← desktop | `&&F%%` | Final ack (comando procesado) | — |

*Legacy* (formato línea, firmware antiguo): `N <nombre>`, `U <id>`, `B <bat>`.

## Cómo mantener los HEX sincronizados

```bash
# 1. Clonar zowiLibs (si no se tiene):
#    git clone https://github.com/eduardomillan/zowiLibs.git
# 2. Ejecutar el script de sincronización:
scripts/sync_firmware_from_zowiLibs.sh [/ruta/a/zowiLibs]
# 3. Verificar el diff, recompilar y probar.
```

El script copia los HEX desde zowiLibs a `src/firmware/` y normaliza los saltos de
línea (CRLF→LF). Los HEX se mantienen commiteados en ZowiDesktop para que el
proyecto sea autónomo en el build; el script es el mecanismo reproducible de
mantener la sincronización con la fuente real.
