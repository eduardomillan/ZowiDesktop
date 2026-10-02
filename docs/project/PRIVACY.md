# Política de privacidad / Privacy policy

> **BORRADOR — pendiente de revisión.** Este texto describe lo que la aplicación
> hace realmente a fecha de hoy (comprobado en el código). Antes de publicarlo
> hay que: (1) rellenar el responsable y el contacto (`[...]`), (2) que lo
> revise la Delegación de Protección de Datos de la Generalitat o un asesor
> jurídico, y (3) decidir los puntos marcados como *por definir* de la sección
> del ranking online. No es asesoramiento jurídico.
>
> **DRAFT — pending review.** This text describes what the app actually does
> today (checked against the code). Before publishing: fill in the controller and
> contact (`[...]`), have it reviewed by a data-protection officer or legal
> adviser, and settle the *to be defined* points of the online-ranking section.
> Not legal advice.

---

# Español

**Última actualización:** [FECHA] · **Responsable:** [NOMBRE / ENTIDAD] ·
**Contacto:** [CORREO]

## 1. Resumen

Zowi Desktop funciona **en tu ordenador**. No tiene cuentas de usuario, ni
publicidad, ni analítica, ni perfiles, y **no envía datos a Internet** (salvo el ranking online opcional de la sección 6, que solo se activa si lo marcas). La
aplicación no te pide nombre, edad, correo ni ningún otro dato personal.

## 2. Qué se guarda en tu equipo

Todo se guarda en archivos de tu carpeta de usuario y no sale de ahí:

| Dato | Para qué | Dónde |
|------|----------|-------|
| Dirección Bluetooth o puerto USB del Zowi, su nombre, versión de firmware, batería y tipo de conexión | Recordar tu robot para reconectar | Archivo de sesión `ZowiApp.json` |
| Idioma, avisos de ayuda ya vistos, última puntuación de Memory y de Pintabocas, y la secuencia que montas en Timeline | Recordar tus ajustes y tu trabajo | `ZowiApp.json` |
| Preferencias de proyectos | Recordar tus ajustes | Carpeta de datos de Zowi Desktop |
| Jugadores del ranking (`Player-123`, un número aleatorio de 3 cifras), sus mejores puntuaciones y la fecha del último cambio | Mostrar el ranking local | `ZowiRanking.json` |
| Registros técnicos (logs): mensajes de funcionamiento, que pueden incluir la dirección Bluetooth y el nombre del robot y los mensajes intercambiados con él | Diagnosticar fallos | `ZowiDesktop-AAAA-MM-DD.log` (no se borran solos) |

Ubicación habitual: en Linux, `~/.config/ZowiDesktop/` (sesión y ranking) y
`~/.local/share/ZowiDesktop/` (logs); en Windows, `%APPDATA%\ZowiDesktop\` y
`%LOCALAPPDATA%\ZowiDesktop\`.

Los jugadores del ranking son **números**, no personas: no se asocian a ningún
nombre real. El administrador de una instalación (por ejemplo, un centro educativo)
puede **desactivar el ranking por completo**; entonces no se muestra ni se guarda nada. El nombre que pongas a tu Zowi también se guarda en tu equipo:
evita usar nombres reales o datos personales.

## 3. Qué no hace la aplicación

- No usa cámara, micrófono ni ubicación.
- No contiene publicidad, compras ni analítica, y no hace seguimiento.
- No envía datos a Internet (salvo el ranking online opcional, sección 6). La única comunicación es **Bluetooth o USB con tu
  Zowi** (la app busca robots cercanos para emparejarlo).
- Cuando abres un enlace (web del proyecto, proyectos, ayuda) se abre tu
  **navegador**; el sitio visitado trata tus datos según su propia política.

## 4. Menores y centros educativos

La aplicación no solicita datos personales del alumnado y no crea perfiles.
Si se usa en un centro educativo, se recomienda no escribir datos personales
(nombres reales, correos, etc.) en los nombres del robot.

## 5. Cómo borrar tus datos

Cierra la aplicación y borra las carpetas de la tabla anterior (o solo el
archivo que quieras). Dentro de la aplicación **no hay un botón para borrar el
ranking**: lo puede vaciar el administrador con `zowi_cli ranking clear`, y tú
puedes borrar el archivo `ZowiRanking.json` de tu carpeta de usuario.

## 6. Ranking online (*todavía no activo; texto preparado*)

> Esta sección **no se aplica** hasta que se publique el ranking online. Es opcional
> y solo se activa si lo aceptas (casilla desmarcada por defecto).
>
> **Hipótesis de trabajo (pendiente de consulta a la Delegación de Protección de Datos
> de la Generalitat):** el número `Player-NNN` es aleatorio, no se pide ni se guarda
> ningún dato del alumnado y no hay relación entre el número y la persona; por tanto se
> entiende que no hay tratamiento de datos personales. No se afirma cumplimiento
> normativo hasta tener esa respuesta.

- **Qué se enviaría:** tu `Player-NNN`, tu puntuación total (normalizada) y un
  código secreto de propiedad que solo sirve para poder actualizar tu entrada.
  Nada más: ni nombre, ni hora, ni historial.
- **Qué se publica:** el `Player-NNN` y el total, en una lista pública de los 100
  mejores.
- **Cuánto se conserva:** una entrada caduca a los **30 días** sin mejorar la
  puntuación o si sales del top 100. El historial del repositorio donde se publica
  puede conservar entradas antiguas (`Player-NNN` y total, sin datos personales).
- **Base legal:** tu consentimiento. En menores de 14 años lo darían sus padres o
  tutores — *mecanismo por definir*.
- **Terceros:** la lista se publica en GitHub (GitHub Pages) y las
  actualizaciones pasan por un servicio intermedio — *proveedor y país por definir*.
  Ese servicio (Cloudflare) ve tu dirección IP al recibir la petición; solo guarda una
  huella (hash) de ella en un contador diario para limitar abusos, que se borra a los 2 días.
- **Tus derechos:** acceder, rectificar o borrar tu entrada escribiendo a
  [CORREO]; también puedes dejar de enviar puntuaciones y la entrada caducará.

## 7. Cambios

Si esta política cambia, se actualizará la fecha de arriba y se avisará en la
documentación del proyecto.

---

# English

**Last updated:** [DATE] · **Controller:** [NAME / ORGANISATION] ·
**Contact:** [EMAIL]

## 1. Summary

Zowi Desktop runs **on your computer**. It has no user accounts, no advertising,
no analytics and no profiling, and it **does not send data to the Internet** (except the optional online ranking in section 6, which only turns on if you tick it). The
app never asks for your name, age, email or any other personal data.

## 2. What is stored on your computer

Everything is stored in files in your user folder and stays there:

| Data | Purpose | Where |
|------|---------|-------|
| Bluetooth address or USB port of the Zowi, its name, firmware version, battery and connection type | Remember your robot to reconnect | Session file `ZowiApp.json` |
| Language, help notices already seen, last Memory and Mouths scores, and the sequence you build in Timeline | Remember your settings and work | `ZowiApp.json` |
| Project preferences | Remember your settings | Zowi Desktop data folder |
| Ranking players (`Player-123`, a random 3-digit number), their best scores and the date of the last change | Show the local ranking | `ZowiRanking.json` |
| Technical logs: operation messages that may include the Bluetooth address, the robot name and the messages exchanged with it | Diagnose problems | `ZowiDesktop-YYYY-MM-DD.log` (not deleted automatically) |

Usual locations: on Linux, `~/.config/ZowiDesktop/` (session and ranking) and
`~/.local/share/ZowiDesktop/` (logs); on Windows, `%APPDATA%\ZowiDesktop\` and
`%LOCALAPPDATA%\ZowiDesktop\`.

Ranking players are **numbers**, not people: they are not linked to any real
name. The administrator of an installation (for example a school) can **turn the
ranking off completely**; then nothing is shown or saved. The name you give your Zowi is also stored on your computer: avoid real
names or personal data.

## 3. What the app does not do

- It does not use the camera, microphone or location.
- It has no advertising, purchases or analytics, and does not track you.
- It does not send data to the Internet (except the optional online ranking, section 6). The only communication is **Bluetooth or
  USB with your Zowi** (the app scans for nearby robots to pair with).
- When you open a link (project website, projects, help) your **browser** opens;
  the visited site handles your data under its own policy.

## 4. Children and schools

The app does not request students' personal data and does not build profiles. If
it is used in a school, avoid typing personal data (real names, emails, etc.) in
robot names.

## 5. How to delete your data

Close the app and delete the folders in the table above (or just the file you
want). There is **no button inside the app to delete the ranking**: the
administrator can wipe it with `zowi_cli ranking clear`, and you can delete the
`ZowiRanking.json` file in your user folder.

## 6. Online ranking (*not active yet; text prepared*)

> This section **does not apply** until the online ranking is released. It is
> optional and only turns on if you agree (checkbox off by default).
>
> **Working hypothesis (pending consultation with the Generalitat's data-protection
> office):** the `Player-NNN` number is random, no student data is requested or
> stored, and there is no link between the number and the person, so it is understood
> that no personal data is processed. Compliance is not claimed until that answer is
> received.

- **What would be sent:** your `Player-NNN`, your total (normalised) score and a
  secret ownership code that only lets you update your own entry. Nothing else: no
  name, no time, no history.
- **What is published:** the `Player-NNN` and the total, in a public list of the top
  100.
- **Retention:** an entry expires after **30 days** without improving or when you
  leave the top 100. The history of the repository where it is published may keep
  old entries (`Player-NNN` and total, no personal data).
- **Legal basis:** your consent. For children under 14, their parents or guardians
  would give it — *mechanism to be defined*.
- **Third parties:** the list is published on GitHub (GitHub Pages) and updates go
  through an intermediate service — *provider and country to be defined*. That
  service (Cloudflare) sees your IP address when it receives the request; it only
  keeps a hash of it in a daily counter to limit abuse, deleted after 2 days.
- **Your rights:** access, correct or delete your entry by writing to [EMAIL]; you
  can also stop sending scores and the entry will expire.

## 7. Changes

If this policy changes, the date above will be updated and the change announced in
the project documentation.
