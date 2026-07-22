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
- [ ] En `src/views/main.qml` falta internacionalizar la línea `rootNotice.show("Robot already named \"" + Robot.deviceName + "\". Keeping it.")`.

## Nuevas funcionalidades

- [X] Al "olvidar a Zowi" se debe emitir un renombrado de fábrica (nombre 'Zowi' original), si es posible la conexión al robot.
- [X] Al conectar con un Zowi nuevo, es posible que tenga almacenado un nombre distinto al default, por una conexión anterior. En ese caso, el wizard no pedirá renombrarlo y saltará a la homescreen directamente.
- [ ] Añadir un comando en `zowi_cli session` llamado `clear` que borre todos los valores de sesión almacenados.