# CASUÍSTICAS — Review / Testing TODO

Seguimiento de pruebas de la gestión de transporte (Bluetooth / USB) y estados
del robot en ZowiDesktop 0.6.0 (en pruebas).

Formato: `[ ]` = pendiente de revisar · `[x]` = revisado y funciona.
Añade una `x` cuando hayas comprobado la casuística en ejecución.

Leyenda de estados de la máquina de situación:
`Demo` · `Unregistered` · `Connecting` · `Connected` · `Disconnected` · `TransportLost`

---

## A. Según transportes disponibles (arranque, sin robot registrado)

- [ ] **A1 — Solo Bluetooth disponible**: lanza el asistente de emparejamiento Bt (flujo original).
- [ ] **A2 — Solo USB disponible**: sin pairing; el registro se simplifica a detectar puerto serie y confirmar conexión.
- [ ] **A3 — Ambos disponibles**: Bt es prioritario por defecto (`TransportAuto` → Bt). Si el robot aparece también por USB, se advierte y se recomienda desconectar el cable.
- [ ] **A4 — Ninguno disponible**: avisa y arranca en *modo demo*; acciones que requieren hardware deshabilitadas/simuladas.

## B. Según registro previo de Zowi

- [ ] **B1 — No registrado + hay Bt**: asistente de emparejamiento Bt.
- [ ] **B2 — No registrado + solo USB**: registro por USB (sin pairing).
- [ ] **B3 — Registrado por Bt**: se descarta USB; reconexión automática al dispositivo guardado (`activeZowiDeviceAddress`). Cambiar a USB exige *olvidar a Zowi*.
- [ ] **B4 — Registrado por USB**: prioriza reconexión por USB. Pasar a Bt exige *olvidar a Zowi*.
- [ ] **B5 — Registrado por transporte ya no disponible** (p.ej. registrado por Bt, sin adaptador Bt): avisa de que el transporte no está disponible; ofrece reintentar / olvidar y registrar por el disponible / modo demo.

## C. Según resultado de la conexión

- [ ] **C1 — Registrado + conecta**: caso feliz → Home con robot operativo.
- [ ] **C2 — Registrado + no conecta** (apagado, fuera de alcance, cable suelto, adaptador ocupado): estado *desconectado* con opción de reintentar; se conserva el registro; si el otro transporte está disponible, se sugiere.
- [ ] **C3 — No registrado + conecta durante el asistente**: completa el registro (dirección/nombre/appId) y sigue a renombrado/Home.
- [ ] **C4 — No registrado + no conecta durante el asistente**: permite reintentar scan/selección o cancelar y quedar en *modo demo*.
- [ ] **C5 — Conexión perdida en caliente**: degrada a *desconectado* con reintento automático; mantiene el registro.

## D. Casos límite y conflictos

- [ ] **D1 — Doble presencia del mismo robot (Bt + USB a la vez)**: un único transporte activo (Bt prioritario) y aviso del cable.
- [ ] **D2 — Robot USB distinto al registrado por Bt**: no autoconecta; pregunta si *olvidar* el actual y registrar el conectado por USB.
- [ ] **D3 — Transporte aparece/desaparece en runtime** (enchufar cable, activar/desactivar Bt): reevaluación sin reiniciar la app (`usbAvailable`, adaptador Bt, `situation`).
- [ ] **D4 — Modo demo con transporte que aparece después**: ofrece salir del demo e iniciar el registro correspondiente.

## E. Robot rehecho / renombrado previamente (detectado en pruebas)

- [ ] **E1 — Olvidar no toca el robot**: *olvidar a Zowi* solo borra datos de app y desempareja a nivel de sistema; no reescribe la EEPROM (ni nombre ni firmware).
- [ ] **E2 — Registro con nombre ≠ defecto**: al volver a registrar, si el nombre leído no es el nombre por defecto (`zowi_default_name`, case-insensitive) **se salta `WizardRenameScreen`** y va a `HomeScreen`, conservando el nombre.
- [ ] **E3 — Aviso de nombre existente**: al saltar el rename se muestra un `MessageBar` indicando el nombre que ya tenía el robot ("Robot already named \"X\". Keeping it.").
- [ ] **E4 — Lectura y persistencia del firmware (appId)**: se parsea `&&I <appId>%%`, se expone `Robot.appId`, y se persiste en sesión `activeZowiAppId`.
- [ ] **E5 — El appId se muestra en la UI**: pill `FW <appId>` en `StatusBar` (cuando conectado) y línea *Firmware (appId)* en `DevOverlay`.

## F. Barra de título y versión

- [ ] **F1 — Título con versión**: `ZowiDesktop - {versión} - {pantalla}` (versión desde `PROJECT_VERSION` → `ZOWI_VERSION` → `AppVersion`).
- [ ] **F2 — Todas las pantallas definen `screenName`**: el título nunca queda con guion final suelto.

## G. Renombrado de controlador (0.6.0)

- [ ] **G1 — `BluetoothController` → `RobotController`** y propiedad QML `Bluetooth` → `Robot`, sin romper backends Qt (`QtBluetoothBackend`, `QBluetooth*`).

---

## Notas de verificación

- Compilar: `./build.sh --gui`
- Arranque headless de humo: `QT_QPA_PLATFORM=offscreen ./build/src/gui/ZowiDesktop`
- El transporte con el que se registró el Zowi se persiste en `activeZowiTransport`
  (bt/usb); cambiarlo requiere *olvidar a Zowi*.
- Sesión (GUI/CLI deben coincidir): `activeZowiDeviceAddress`, `activeZowiName`,
  `activeZowiAppId`, `activeZowiBattery`, `activeZowiTransport`, `wizardDismissed`.
