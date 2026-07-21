# TODO LIST

## Bugs detectados

- [ ] Al iniciar ZowiDesktop, se queda todo el rato en "Conectando..." la statusbar. Debería intentar la conexión según el transport que tenga configurado, durante un tiempo limitado por timeout. Si no es posible la conexión (auto, usb o bluetooth) debería indicarlo en dicha statusbar.
- [X] En docs/tests hay un ZOWILIB_HOWTO, debería referenciarse con TEST_ al inicio.
- [ ] Si el robot tiene el firmware modificado (por bitbloq, por ejemplo), el proceso de renombrado falla


## Nuevas funcionalidades

- [ ] Al "olvidar a Zowi" se debe emitir un renombrado de fábrica (nombre 'Zowi' original), si es posible la conexión al robot.
- [ ] Al conectar con un Zowi nuevo, es posible que tenga almacenado un nombre distinto al default, por una conexión anterior. En ese caso, el wizard no pedirá renombrarlo y saltará a la homescreen directamente.