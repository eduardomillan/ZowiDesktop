# Gestión del 'transport' Bluetooth / USB
ZowiDesktop está basado en la app Android original de Zowi publicada por BQ, allá por el año 2015. En su idea original, solamente se contemplaba la conexión Bluetooth al robot, no se pensó en ninguna otra forma.

Durante el desarrollo de ZowiDesktop, surge la duda de si sería posible conectar también mediante cable USB. Esto sería útil en caso de que el PC donde se quiera trabajar con la aplicación no dispusiera de dispositivos Bluetooth, estuviese apagado o no se pudiese utilizar.

Entonces, surgen diversas **CASUÍSTICAS**:

- En caso de existir ambas opciones, ¿cuál sería la prioritaria? Lo lógico sería usar Bt, pero el inconveniente sería si, a la vez, se conecta al robot por USB. Debería advertirse al usuario de esta situación y recomendarle desconectar el cable USB y trabajar exclusivamente por Bt.
- Sin ningún robot registrado, debería intentarse el transporte por Bt, lo cual supone realizar la fase de emparejamiento ya programada en la app original. En cambio, si no hay Bt pero hay USB, no existe el concepto de *emparejamiento*.
- Si ya se ha registrado el robot por Bt, debería descartarse ya el transporte USB. Si el usuario desea cambiarlo, debería *olvidar a Zowi* y empezar de nuevo.
- Al iniciar la aplicación sin Zowi registrado, si no hay Bt y hay USB, el proceso de registro de Zowi cambia.
- Si no hay Bt ni USB, advertir al usuario y trabajar en *modo demo*.
- Otras casuísticas que se pueden dar, y que se analizan a continuación.


## Matriz de casuísticas

Para razonar de forma exhaustiva conviene combinar tres ejes independientes:

1. **Disponibilidad de transportes**: `Bt`, `USB`, `ambos` o `ninguno`.
2. **Registro previo de Zowi**: `no registrado` o `registrado` (y en este caso, registrado *por Bt* o *por USB*).
3. **Resultado de la conexión**: `conecta` o `no conecta`.

### A. Según transportes disponibles (arranque, sin robot registrado)

- **Solo Bt disponible**: flujo original. Se lanza el asistente de emparejamiento por Bluetooth. Es el camino de referencia heredado de la app de BQ.
- **Solo USB disponible**: no aplica el concepto de *emparejamiento*. El registro se simplifica a detectar el puerto/dispositivo serie y confirmar la conexión. El asistente debe ocultar los pasos de scan/pairing Bt y mostrar en su lugar la selección/confirmación del cable USB.
- **Ambos disponibles**: Bt es el transporte prioritario por defecto (`TransportAuto` resuelve a Bt). Si además se detecta el robot por USB simultáneamente, advertir al usuario y recomendar desconectar el cable para trabajar solo por Bt, evitando conflictos de doble conexión al mismo robot.
- **Ninguno disponible**: advertir (`no_bluetooth_demo`) y arrancar en *modo demo* (sin robot real). Toda acción que requiera hardware debe deshabilitarse o simularse.

### B. Según registro previo de Zowi

- **No registrado + hay Bt**: iniciar asistente de emparejamiento Bt.
- **No registrado + solo USB**: iniciar registro por USB (sin pairing).
- **Registrado por Bt**: descartar USB como transporte. Reconexión automática al dispositivo guardado (`activeZowiDeviceAddress`). Para cambiar a USB, el usuario debe *olvidar a Zowi* y volver a registrar.
- **Registrado por USB**: análogamente, priorizar la reconexión por USB. Si el usuario quiere pasar a Bt, *olvidar a Zowi* y rehacer el registro/emparejamiento.
- **Registrado por un transporte que ya no está disponible** (p. ej. registrado por Bt pero al arrancar no hay adaptador Bt): informar de que el transporte del robot registrado no está disponible. Ofrecer: (a) reintentar cuando vuelva el transporte, (b) *olvidar a Zowi* y registrar por el transporte disponible, o (c) entrar en *modo demo*.

### C. Según resultado de la conexión

- **Registrado + conecta**: caso feliz, ir directamente a Home con el robot operativo.
- **Registrado + no conecta** (robot apagado, fuera de alcance, cable desconectado, adaptador ocupado): mostrar estado *desconectado* con opción de reintentar. No forzar un re-emparejamiento; conservar el registro. Si el otro transporte está disponible, sugerirlo como alternativa (pero sin cambiar el registro salvo confirmación del usuario).
- **No registrado + conecta durante el asistente**: completar el registro (guardar dirección/nombre/appId) y continuar al paso de renombrado/Home.
- **No registrado + no conecta durante el asistente**: permitir reintentar el scan/selección, o cancelar y quedar en *modo demo* hasta que haya un robot disponible.
- **Conexión que se pierde en caliente** (se desconecta el robot ya conectado): degradar a estado *desconectado* con reintento automático; mantener el registro y no expulsar al usuario de la app.

### D. Casos límite y conflictos

- **Doble presencia del mismo robot** (Bt + USB a la vez): elegir un único transporte activo (Bt por prioridad) y advertir del cable.
- **Robot USB distinto al registrado por Bt**: no autoconectar; preguntar al usuario si desea *olvidar* el actual y registrar el conectado por USB.
- **Transporte aparece/desaparece en runtime** (se enchufa el cable, se activa/desactiva Bt): reevaluar disponibilidad y actualizar la UI (`usbAvailable`, estado del adaptador Bt) sin necesidad de reiniciar la app.
- **Modo demo con transporte que aparece después**: al detectar Bt o USB tras haber entrado en demo, ofrecer salir del modo demo e iniciar el registro correspondiente.

### E. Robot rehecho / renombrado previamente (casuística detectada en pruebas)

- **Situación**: al *olvidar a Zowi* no se toca el robot, así que un Zowi que
  fue renombrado (o con firmware modificado) en una conexión anterior conserva
  su nombre y su firmware en su EEPROM.
- **Nombre**: al volver a registrar, si el nombre leído **no es el nombre por
  defecto** (`zowi_default_name` de `config.json`, p.ej. "Zowi"/"OpenZowi",
  comparación case-insensitive) se **salta `WizardRenameScreen`** y se navega
  directo a `HomeScreen`, conservando el nombre actual. Se muestra un aviso
  (`MessageBar`) indicando el nombre que ya tenía.
- **Firmware (appId)**: no se restaura ni se hace nada sobre el robot. Se **lee**
  el `appId` que el robot reporta (`&&I <appId>%%`) y se **persiste** en la
  sesión (`activeZowiAppId`) para saber con qué firmware trabaja. Más adelante
  se indicará en la UI; de momento ya se muestra en la `StatusBar` (pill `FW`)
  y en el `DevOverlay` (línea *Firmware (appId)*).
- **Regla de oro**: *olvidar* solo borra los datos de la app y desempareja a
  nivel de sistema; nunca reescribe la EEPROM del robot (ni nombre ni firmware).

## Máquina de estados

Decisión de diseño: el transporte deja de ser una *preferencia* que el usuario fija
a mano y pasa a ser una *consecuencia* del estado (transportes disponibles +
registro + resultado de conexión). El código resuelve la situación y ofrece solo
**acciones contextuales**.

Entradas (señales que reevalúan el estado):
- `btAvail` / `usbAvail`: disponibilidad de cada transporte (hotplug en runtime).
- `registered`: existe `activeZowiDeviceAddress`.
- `regTransport`: transporte del registro (`activeZowiTransport` = bt | usb).
- `connected`: hay enlace vivo con el robot.

Estados y transiciones:

```
                         ┌──────────────────────────────────────────────┐
                         │                   START                       │
                         │        (arranque / refreshTransports)         │
                         └──────────────────────┬───────────────────────┘
                                                │
                        registered? ────────────┴───────────── no
                            │ sí                                 │
                            ▼                                    ▼
                 ┌─────────────────────┐              ┌─────────────────────┐
                 │  regTransport avail? │              │  btAvail || usbAvail │
                 └───────┬─────────┬────┘              └──────┬──────────┬───┘
                     sí  │         │ no                    sí │          │ no
                         ▼         ▼                         ▼          ▼
                 ┌───────────┐ ┌──────────────┐    ┌──────────────┐ ┌────────┐
                 │CONNECTING │ │TRANSPORT_LOST│    │ UNREGISTERED │ │  DEMO  │
                 └─────┬─────┘ │ (reintentar/ │    │  (asistente: │ │(sin hw;│
                       │       │  olvidar/    │    │  bt=pairing, │ │ acción:│
              conecta? │       │  demo)       │    │  usb=directo)│ │ salir  │
               ┌───────┴────┐  └──────────────┘    └──────┬───────┘ │ si apa-│
           sí  │            │ no                          │         │ rece hw│
               ▼            ▼                       conecta│         └────┬───┘
        ┌────────────┐ ┌──────────────┐             ┌──────┴─────┐       │
        │ CONNECTED  │ │ DISCONNECTED │             │  guarda     │  btAvail||
        │  (Home)    │ │ (reintentar/ │             │  registro + │  usbAvail
        └─────┬──────┘ │  otro transp.│             │  regTransport│  ────────┘
              │        │  si confirma)│             └──────┬──────┘   (a UNREGISTERED)
   pierde     │        └──────┬───────┘                    │
   enlace     │               │ reintenta                  ▼
              ▼               └────────► CONNECTING     CONNECTED
        ┌──────────────┐
        │ DISCONNECTED │  (reconexión automática; conserva registro)
        └──────────────┘
```

Acciones contextuales ofrecidas por estado:

| Estado          | Acciones disponibles                                             |
|-----------------|-----------------------------------------------------------------|
| `CONNECTED`     | (ninguna de transporte) opciones normales de la app             |
| `CONNECTING`    | cancelar                                                         |
| `DISCONNECTED`  | reintentar; olvidar a Zowi; (sugerir otro transporte si avail)  |
| `TRANSPORT_LOST`| reintentar cuando vuelva; olvidar y registrar por el disponible; modo demo |
| `UNREGISTERED`  | iniciar registro (bt=pairing / usb=directo) según disponibilidad |
| `DEMO`          | (solo demo) al aparecer bt/usb: salir de demo e iniciar registro |

Reglas invariantes:
- Con Zowi registrado, el transporte queda ligado a `regTransport`. Cambiarlo exige
  *olvidar a Zowi* (no hay cambio manual de transporte en caliente).
- Prioridad al detectar ambos sin registro: USB si hay un Zowi probado en puerto;
  si no, Bt. Advertir del cable si el mismo robot aparece por Bt y USB a la vez.
- Cualquier cambio de `btAvail`/`usbAvail`/`connected` reevalúa el estado y
  actualiza las acciones sin reiniciar la app.

