# Seguridad del token de GitHub en el remote `origin`

> Fecha: 2026-09-08. Nota creada a raíz de la revisión del remote de este repo.

## Qué pasó

El remote `origin` de este repositorio usaba una URL con un token de acceso
de GitHub incrustado:

```
https://<usuario>:<token-ghp_...>@github.com/eduardomillan/ZowiDesktop.git
```

Ese token es una llave maestra de la cuenta: quien lo tenga puede hacer push a
los repos, leerlos, borrarlos, etc.

## Por qué es un riesgo

1. **No se ha commiteado ni subido a GitHub** — el token no está en los commits
   del repositorio, así que no está publicado allí. Esto es lo bueno.
2. **Pero sí queda expuesto localmente**:
   - Aparece con `git remote -v` (quedó además impreso en outputs de sesiones
     anteriores, por lo que debe considerarse *filtrado*).
   - Cualquiera que copie la carpeta del proyecto, lea `.git/config` o recopile
     el historial de comandos se lleva la llave.
   - Vive en texto plano, incrustado en la URL, en lugar de en un almacén
     seguro de credenciales.

Regla práctica: cuando un token aparece en un log, un chat o un archivo, se
considera **filtrado**, y lo correcto es **anularlo** y regenerarlo.

## Qué hacer (orden recomendado)

1. **Revocar/regenerar el token en GitHub**: Configuración de GitHub →
   *Developer settings* → *Personal access tokens* → revoke o regenerate el
   token expuesto. Esto deja inservible el que quedó en configs/logs.
2. **Quitar el token de la URL** (hecho en este repo):
   ```
   git remote set-url origin https://github.com/eduardomillan/ZowiDesktop.git
   ```
3. **Autenticarse sin escribir tokens**: usar `gh auth login` (GitHub CLI) o el
   credential helper del sistema (`git config --global credential.helper`, por
   ejemplo `libsecret` en Linux). Git pide credenciales una vez y las guarda en
   el llavero del sistema.

## Referencia de qué es un credential helper (para tontos)

### La idea en una frase

Git necesita una "contraseña" cada vez que interactúa con GitHub (fetch, push,
clone…) para demostrar quién eres. Un *credential helper* es el encargado de
**guardar y facilitar esa contraseña en una caja fuerte segura**, para que tú no
tengas que escribirla en cada comando ni (mucho menos) dejarla escrita en
archivos o URLs.

### La analogía del portero

Piensa en tu cuenta de GitHub como un edificio con un portero
(Git). Antes de dejarte subir, el portero te pide el carné:

- **Sin helper**: tienes que sacar el carné cada vez. Además, a la gente "lista"
  se le ocurrió dejar el carné escrito en la puerta (URL con el token) — justo lo
  que pasó en este repo. Pésima idea: cualquiera que pase lo ve.
- **Con helper**: el carné está guardado en la caja fuerte del edificio (el
  llavero del sistema operativo). El portero lo coge por ti la primera vez que
  lo necesitas, lo guarda en la caja fuerte, y a partir de entonces ya no tienes
  que volver a enseñarlo. Tú solo verificas una vez (como firmar al recibir un
  paquete) y listo.

### Qué hace exactamente y cuándo

1. La primera vez que haces `git push` (o clone/fetch), Git necesita credenciales.
2. Git pregunta al credential helper: "¿tienes guardadas credenciales para
   github.com?".
   - **Sí** → el helper las entrega, Git las usa y sigue su camino. Nunca las ves.
   - **No** → Git te las pide en la terminal (a veces con una ventanita del
     sistema: "Introduce tu usuario y contraseña"), y **el helper se encarga de
     guardarlas** en la caja fuerte para la próxima vez.
3. A partir de ese momento, el flujo es automático: las credenciales viven en el
   llavero del sistema **cifradas**, no en el `.git/config` ni en el historial.

### Tipos de helper (de peor a mejor)

| Helper | Cómo guarda | ¿Seguro? | ¿Para qué sirve? |
|---|---|---|---|
| `store` | Fichero de texto plano en `~/.git-credentials` | 🔴 NO — es justo lo que queremos evitar | Solo para entornos temporales/sin interfaz |
| `cache` | En memoria (RAM) durante un tiempo limitado | 🟡 Media — desaparece al apagar | Para no escribir cada vez durante una sesión larga |
| `libsecret` / `secret-service` | En el llavero de GNOME/KDE (Keyring) | 🟢 Sí — cifrado por el sistema | **Recomendado** en Linux de escritorio |
| `osxkeychain` | En el llavero de macOS (Keychain) | 🟢 Sí | **Recomendado** en macOS |
| Git Credential Manager | En el almacén seguro de Windows / winget | 🟢 Sí | **Recomendado** en Windows (viene por defecto) |

### Cómo ver qué helper tienes ahora

```bash
git config --global --get credential.helper   # muestra el helper global (si existe)
git config --get credential.helper           # el del repo actual
```

Si no sale nada, no tienes ninguno configurado: Git te pedirá credenciales en
cada operación (y con una URL sin token, esto es lo que verás al hacer push).

### Cómo configurarlo

**Linux (GNOME/KDE)**, usa el llavero con `libsecret`:

```bash
sudo apt install libsecret-1-0 libsecret-1-dev
git config --global credential.helper libsecret
```

**Alternativa simple** (guarda en texto plano, solo si aceptas el riesgo):

```bash
git config --global credential.helper store      # ⚠️ texto plano en ~/.git-credentials
```

**macOS**:

```bash
git config --global credential.helper osxkeychain
```

**Windows**: Git for Windows ya trae *Git Credential Manager* configurado por
defecto; no hay que hacer nada.

### La alternativa más cómoda: GitHub CLI (`gh`)

Instala `gh` (GitHub CLI) y autentícate una vez:

```bash
sudo apt install gh        # Linux
brew install gh            # macOS
# o descargarlo desde https://cli.github.com

gh auth login              # te abre el navegador, inicias sesión una vez
gh auth setup-git          # hace que Git use las credenciales de gh
```

A partir de ahí, `git push`, `git fetch`, etc. funcionan sin escribir nada:
Git delega en `gh` la identificación. Además `gh` te permite revocar fácilmente
el logueo (`gh auth logout`) si un día dejas de usar la máquina.

### Qué verás al hacer push después de limpiar la URL

1. Si tienes helper o `gh` configurado → funciona igual que siempre (pide
   credenciales la primera vez, luego ya no).
2. Si no tienes nada → Git imprimirá un error pidiendo credenciales o bien te las
   pedirá en la terminal:
   ```
   Username for 'https://github.com': …
   Password for 'https://eduardo@github.com': …
   ```
   En "Password" se introduce **el token** (no la contraseña de la cuenta) — y si
   el helper está bien configurado, quedará guardado en el llavero, no en URLs.

### Resumen para el día a día

- El token **nunca** debe ir dentro de una URL de remote, ni en un commit, ni en
  un README. Si aparece en cualquier log/chat/archivo → **revócalo ya**.
- Configura una vez el credential helper del sistema (o `gh auth login`).
- Las credenciales se quedan cifradas en el llavero y tú no vuelves a pensar en
  ello.