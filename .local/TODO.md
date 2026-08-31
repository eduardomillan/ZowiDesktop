# TODO LIST

## Bugs detectados

- [X] Al iniciar ZowiDesktop, se queda todo el rato en "Conectando..." la statusbar. Debería intentar la conexión según el transport que tenga configurado, durante un tiempo limitado por timeout. Si no es posible la conexión (auto, usb o bluetooth) debería indicarlo en dicha statusbar.
- [X] En docs/tests hay un ZOWILIB_HOWTO, debería referenciarse con TEST_ al inicio.
- [ ] Si el robot tiene el firmware modificado (por bitbloq, por ejemplo), el proceso de renombrado falla
- [X] Si se intenta 'renombrar' a Zowi con el mismo nombre que ya tiene, el proceso de renombrado falla
- [X] Al iniciar la aplicación con robot registrado, no muestra el porcentaje de batería
- [X] En Zowi CLI, comando 'control', se mapean las teclas con las teclas de cursor y también con las teclas A,W,S,D,Q,E de la siguiente forma:
  - Adelante: Flecha arriba del cursor o W. Corresponde al movimiento WALK FORWARD
  - Atrás: Flecha abajo del cursor o S. Movimiento: WALK BACKWARD
  - Izquierda: Flecha izquierda del cursor o A. Movimiento MOONWALKER LEFT.
  - Derecha: Flecha derecha del cursor o D. Movimiento: MOONWALKER_RIGHT.
  - Giro izquierda. Tecla Q. Movimiento: TURN_LEFT
  - Giro derecha. Tecla E. Movimiento: TURN_RIGHT
- [X] En `src/views/main.qml` falta internacionalizar la línea `rootNotice.show("Robot already named \"" + Robot.deviceName + "\". Keeping it.")`.
- [X] Cuando no hay disponible ningún robot ni por USB ni Bt y hay un robot registrado, el programa se queda en "Conectando..." por siempre, cuando debería haber un timeout de unos 10 segundos y pasar a "modo demo".
- [X] Cuando no se encuentra ningún robot en la ventana de escaneo, no debería mostrarse la lista de robots. Solamente debe aparecer esta lista cuando se encuentre al menos un robot. Si al cabo de un tiempo no se ha encontrado ningún robot, que lo indique mediante un mensaje en ese mismo lugar. 


## Nuevas funcionalidades

- [X] Al "olvidar a Zowi" se debe emitir un renombrado de fábrica (nombre 'Zowi' original), si es posible la conexión al robot.
- [X] Al conectar con un Zowi nuevo, es posible que tenga almacenado un nombre distinto al default, por una conexión anterior. En ese caso, el wizard no pedirá renombrarlo y saltará a la homescreen directamente.
- [X] Añadir un comando en `zowi_cli session` llamado `clear` que borre todos los valores de sesión almacenados.
- [X] La ventana DEV se puede mostrar/ocultar con Ctrl+D en cualquier screen. Es un panel contenido dentro de la ventana principal, movible y redimensionable en 4 lados/esquinas, con geometría persistente (se centraliza en main.qml, no por pantalla).


## Pruebas automatizadas
- [ ] Smoke test de la GUI headless (arranque/cierre con `QT_QPA_PLATFORM=offscreen`), fuera del alcance de la fase inicial de pruebas de caja negra del CLI.

## Windows
- [ ] Implementar la conexión por USB y probar todo el flujo de la aplicación