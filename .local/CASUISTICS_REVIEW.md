# CASUÍSTICAS — Review / Testing TODO

Seguimiento de pruebas de la gestión de transporte (Bluetooth / USB) y estados
del robot en ZowiDesktop 0.6.0 (en pruebas).

Formato: `[ ]` = pendiente de revisar · `[x]` = revisado y funciona.
Añade una `x` cuando hayas comprobado la casuística en ejecución.

Leyenda de estados de la máquina de situación:
`Demo` · `Unregistered` · `Connecting` · `Connected` · `Disconnected` · `TransportLost`

---

## A. Según transportes disponibles (arranque, sin robot registrado)

- [X] **A1 — Solo Bluetooth disponible**: lanza el asistente de emparejamiento Bt (flujo original).
- [X] **A2 — Solo USB disponible**: sin pairing; el registro se simplifica a detectar puerto serie y confirmar conexión.
- [X] **A3 — Ambos disponibles**: Bt es prioritario por defecto (`TransportAuto` → Bt). Si el robot aparece también por USB, se advierte y se recomienda desconectar el cable.
- [X] **A4 — Ninguno disponible**: avisa y arranca en *modo demo*; acciones que requieren hardware deshabilitadas/simuladas.

## B. Según registro previo de Zowi

- [X] **B1 — No registrado + hay Bt**: asistente de emparejamiento Bt.
- [X] **B2 — No registrado + solo USB**: registro por USB (sin pairing).
- [X] **B3 — Registrado por Bt**: se descarta USB; reconexión automática al dispositivo guardado (`activeZowiDeviceAddress`). Cambiar a USB exige *olvidar a Zowi*.
- [X] **B4 — Registrado por USB**: prioriza reconexión por USB. Pasar a Bt exige *olvidar a Zowi*.
- [X] **B5 — Registrado por transporte ya no disponible** (p.ej. registrado por Bt, sin adaptador Bt): avisa de que el transporte no está disponible; ofrece reintentar / olvidar y registrar por el disponible / modo demo.

## C. Según resultado de la conexión

- [X] **C1 — Registrado + conecta**: caso feliz → Home con robot operativo.
- [X] **C2 — Registrado + no conecta** (apagado, fuera de alcance, cable suelto, adaptador ocupado): estado *desconectado* con opción de reintentar; se conserva el registro; si el otro transporte está disponible, se sugiere.
- [X] **C3 — No registrado + conecta durante el asistente**: completa el registro (dirección/nombre/appId) y sigue a renombrado/Home.
- [X] **C4 — No registrado + no conecta durante el asistente**: permite reintentar scan/selección o cancelar y quedar en *modo demo*.
- [X] **C5 — Conexión perdida en caliente**: degrada a *desconectado* con reintento automático; mantiene el registro.

## D. Casos límite y conflictos

- [X] **D1 — Doble presencia del mismo robot (Bt + USB a la vez)**: un único transporte activo (Bt prioritario) y aviso del cable.
- [ ] **D2 — Robot USB distinto al registrado por Bt**: no autoconecta; pregunta si *olvidar* el actual y registrar el conectado por USB.
- [ ] **D3 — Transporte aparece/desaparece en runtime** (enchufar cable, activar/desactivar Bt): reevaluación sin reiniciar la app (`usbAvailable`, adaptador Bt, `situation`).
- [ ] **D4 — Modo demo con transporte que aparece después**: ofrece salir del demo e iniciar el registro correspondiente.

## E. Robot rehecho / renombrado previamente (detectado en pruebas)

- [ ] **E2 — Registro con nombre ≠ defecto**: al volver a registrar, si el nombre leído no es el nombre por defecto (`zowi_default_name`, case-insensitive) **se salta `WizardRenameScreen`** y va a `HomeScreen`, conservando el nombre.
- [ ] **E3 — Aviso de nombre existente**: al saltar el rename se muestra un `MessageBar` indicando el nombre que ya tenía el robot ("Robot already named \"X\". Keeping it.").
- [ ] **E4 — Lectura y persistencia del firmware (appId)**: se parsea `&&I <appId>%%`, se expone `Robot.appId`, y se persiste en sesión `activeZowiAppId`.
- [ ] **E5 — El appId se muestra en la UI**: pill `FW <appId>` en `StatusBar` (cuando conectado) y línea *Firmware (appId)* en `DevOverlay`.



---

## Notas de verificación

- Compilar: `./build.sh --gui`
- Arranque headless de humo: `QT_QPA_PLATFORM=offscreen ./build/src/gui/ZowiDesktop`
- El transporte con el que se registró el Zowi se persiste en `activeZowiTransport`
  (bt/usb); cambiarlo requiere *olvidar a Zowi*.
- Sesión (GUI/CLI deben coincidir): `activeZowiDeviceAddress`, `activeZowiName`,
  `activeZowiAppId`, `activeZowiBattery`, `activeZowiTransport`, `wizardDismissed`.
