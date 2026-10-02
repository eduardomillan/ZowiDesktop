# Cloudflare + token de GitHub — guía paso a paso (para dummies)

> Para qué sirve: el **ranking mundial** necesita un pequeño "intermediario" en
> Internet que reciba las puntuaciones, compruebe que son razonables y las publique.
> Ese intermediario es un **Cloudflare Worker** (gratis). Para publicar el ranking
> en tu página de GitHub necesita una **llave** (un *token* de GitHub) que se guarda
> **solo en Cloudflare**, nunca en la aplicación ni en el repositorio.
>
> Qué tienes que hacer tú: las partes **A** (cuenta de Cloudflare) y **B** (token
> de GitHub). Qué hago yo: el código del Worker y de la app. Las partes **C** y **D**
> (crear la base de datos y desplegar) las haremos juntos cuando el código esté listo.
>
> Regla de oro: **el token es como una contraseña. No lo pegues nunca en un chat
> (tampoco conmigo), ni en un commit, ni en un documento.** Si se te escapa, ve a la
> sección "Si algo sale mal".

## Índice

- [A. Cuenta de Cloudflare](#a-cuenta-de-cloudflare)
- [B. Token de GitHub](#b-token-de-github)
- [C. Herramientas en tu ordenador](#c-herramientas-en-tu-ordenador)
- [D. Preparar y desplegar el Worker (lo haremos juntos)](#d-preparar-y-desplegar-el-worker-lo-haremos-juntos)
- [Qué me tienes que pasar y qué NO](#qué-me-tienes-que-pasar-y-qué-no)
- [Si algo sale mal](#si-algo-sale-mal)
- [Límites del plan gratuito](#límites-del-plan-gratuito)

---

## A. Cuenta de Cloudflare

No necesitas dominio propio ni tarjeta para el plan gratuito de Workers (si en algún
momento te la pidieran, para este uso **no** hace falta; puedes parar ahí y avisarme).

1. Abre <https://dash.cloudflare.com/sign-up> en el navegador.
2. Escribe tu **correo** y una **contraseña larga** (mejor con un gestor de contraseñas).
   Pulsa *Sign up*.
3. Cloudflare te envía un **correo de verificación**. Ábrelo y pulsa el enlace de
   confirmación.
4. Inicia sesión en <https://dash.cloudflare.com>. Si te ofrece "añadir un sitio/dominio",
   **sáltatelo** (*Skip* / *Go to dashboard*): no hace falta.
5. **Activa la verificación en dos pasos (2FA)**: arriba a la derecha, tu perfil →
   *My Profile* → *Authentication* → *Two-Factor Authentication* → sigue los pasos con tu
   app de autenticación (por ejemplo, la del móvil). Guarda los **códigos de recuperación**.
6. En el menú lateral busca **Workers & Pages** (según la versión del panel puede estar
   dentro de "Compute" o "Build"). Verás que está en el **plan gratuito (Free)**.

Qué deberías ver al terminar: el panel de Cloudflare con *Workers & Pages* disponible y
tu cuenta con 2FA activado.

## B. Token de GitHub

Vamos a crear un token **con los permisos mínimos**: solo para este repositorio y solo
para escribir contenido. Así, aunque se filtrara, no podría tocar tus otros
repositorios ni tu cuenta.

1. Entra en <https://github.com> con tu cuenta (`eduardomillan`).
2. Arriba a la derecha pulsa tu **foto de perfil** → **Settings**.
3. En la barra izquierda, abajo del todo: **Developer settings**.
4. **Personal access tokens** → **Fine-grained tokens** → botón **Generate new token**.
   (Si GitHub te pide la contraseña o un código, introdúcelo.)
5. Rellena el formulario:
   - **Token name:** `zowi-ranking-worker`
   - **Expiration:** elige una fecha (por ejemplo **1 año**). Apunta en tu calendario
     un aviso unos días antes para renovarlo.
   - **Description:** `Worker que publica el ranking mundial de Zowi Desktop`
   - **Resource owner:** `eduardomillan`
   - **Repository access:** marca **Only select repositories** y elige
     **`eduardomillan/ZowiDesktop`** (solo ese).
   - **Permissions → Repository permissions:** busca **Contents** y cámbialo a
     **Read and write**. **No marques nada más.** (*Metadata: Read-only* se añade solo y
     está bien.)
6. Abajo, **Generate token**.
7. GitHub te muestra el token **una sola vez** (empieza por `github_pat_...`).
   **Cópialo ahora** y guárdalo en tu gestor de contraseñas con el nombre
   "Token GitHub - zowi-ranking-worker". **No lo pegues en ningún chat.**

Qué deberías ver al terminar: en *Fine-grained tokens* aparece `zowi-ranking-worker`
con "Repository access: 1 repository" y la fecha de caducidad.

## C. Herramientas en tu ordenador

Necesitas **Node.js** (ya lo tienes: tu equipo tiene `v20.20.2`; sirve cualquier
versión 18 o superior). Comprueba:

```bash
node -v      # debe mostrar v18.x o superior
npm -v
```

**Wrangler** es la herramienta de línea de comandos de Cloudflare. No hace falta
instalarla globalmente: se usa con `npx`:

```bash
npx wrangler --version
```

(La primera vez descarga el programa y puede tardar un poco.)

Ahora **conecta Wrangler con tu cuenta** (se hace una sola vez por ordenador):

```bash
npx wrangler login
```

Se abre el navegador con una pantalla de Cloudflare que pide permiso (*Allow*). Pulsa
**Allow**. Al volver a la terminal verás algo como `Successfully logged in`.
Comprueba con:

```bash
npx wrangler whoami
```

Debe mostrar tu correo y el nombre de tu cuenta.

## D. Preparar y desplegar el Worker (lo haremos juntos)

Cuando el código del Worker esté en el repositorio (`server/ranking-worker/`), los
pasos serán estos. **No tienes que hacerlos todavía**; los dejo aquí para que sepas
qué va a pasar.

1. **Crear la base de datos** (D1, es como una pequeña base SQLite en Cloudflare):

   ```bash
   cd server/ranking-worker
   npx wrangler d1 create zowi-ranking
   ```

   Te muestra un `database_id`. Ese identificador **no es secreto**: lo copio yo al
   archivo `wrangler.toml`.
2. **Crear las tablas** (el archivo `schema.sql` lo preparo yo):

   ```bash
   npx wrangler d1 execute zowi-ranking --remote --file=schema.sql
   ```
3. **Guardar el token de GitHub como secreto del Worker**:

   ```bash
   npx wrangler secret put GITHUB_TOKEN
   ```

   Wrangler te pide el valor: **pega el token** (no se ve mientras lo pegas) y pulsa
   Intro. Se guarda cifrado en Cloudflare; no queda en ningún archivo.
4. **Desplegar**:

   ```bash
   npx wrangler deploy
   ```

   Te devuelve una dirección del tipo `https://zowi-ranking.<tu-subdominio>.workers.dev`.
   La primera vez, Wrangler puede preguntarte qué **subdominio** quieres para tus
   Workers (`<tu-subdominio>.workers.dev`): elige uno corto y fácil, por ejemplo
   `zowi`.
5. **Probar** que responde (sustituye la dirección por la tuya):

   ```bash
   curl -s https://zowi-ranking.<tu-subdominio>.workers.dev/health
   ```

   Debe contestar algo como `{"ok":true}`.
6. **Apuntar la app al Worker**: pones la dirección en la configuración de la
   instalación (clave `ranking_online_submit_url`, la dirección base del Worker sin
   `/submit`) y la del JSON público en `ranking_online_read_url`
   (`https://eduardomillan.github.io/ZowiDesktop/docs/ranking/ranking.json`). Te lo indico yo en su momento.

## Qué me tienes que pasar y qué NO

**Sí** (no son secretos):
- La **dirección del Worker** (`https://...workers.dev`) cuando esté desplegado.
- El **nombre de tu subdominio** de Workers si lo eliges tú.
- Cualquier **mensaje de error** de la terminal (revisa antes que no contenga el token).

**NO** (nunca):
- El **token de GitHub** (`github_pat_...`).
- Tu contraseña o códigos de Cloudflare/GitHub.
- Capturas de pantalla donde se vea el token.

## Si algo sale mal

- **Se me ha escapado el token** (lo pegué en un chat, en un archivo, etc.):
  1. GitHub → Settings → Developer settings → Fine-grained tokens → `zowi-ranking-worker`
     → **Delete** (o *Revoke*). Queda inutilizado al momento.
  2. Crea otro siguiendo la parte B.
  3. Guárdalo en el Worker: `npx wrangler secret put GITHUB_TOKEN`.
  4. Más contexto sobre este tipo de incidentes en `.local/GITHUB_TOKEN_SECURITY.md`.
- **El token ha caducado** (el ranking mundial deja de actualizarse): repite la parte B
  y el paso 3 de D.
- **`wrangler login` no abre el navegador:** copia la dirección que muestra la terminal
  y ábrela a mano.
- **`wrangler deploy` dice que falta algo de D1 o de `wrangler.toml`:** avísame con el
  mensaje exacto; lo arreglo en el repositorio.
- **Quiero parar el ranking mundial ya:** en la configuración de sistema de los equipos
  se pone `ranking_online_allowed` a `false`, o se borra el Worker desde el panel de
  Cloudflare (*Workers & Pages* → tu Worker → *Settings* → *Delete*).

## Límites del plan gratuito

Para este uso sobra:

- **Workers:** 100 000 peticiones al día.
- **Cron (tareas programadas):** hasta 5 por cuenta (usaremos 1 o 2).
- **D1 (base de datos):** 5 millones de filas leídas al día, 100 000 escritas al día y
  5 GB en total. El ranking tiene como mucho 100 filas.
- Los límites se reinician cada día a las 00:00 UTC. Si algún día se superaran, el
  Worker empezaría a rechazar peticiones hasta el reinicio (la app seguiría
  funcionando en local).

Fuentes: documentación de Cloudflare (Workers y D1) y de GitHub (tokens de acceso
personal *fine-grained*).
