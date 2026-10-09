# Actualizar el juego

El repo público es [tonga54/cod2-wasm](https://github.com/tonga54/cod2-wasm).
La rama que se comprueba es `master`. Un push a otra rama no anuncia una
actualización del juego.

## Para quienes juegan en el navegador

El juego comprueba la versión del anfitrión cada minuto mientras la pestaña está
visible. Si el anfitrión instaló una versión nueva, aparece **Recargar** en el
menú principal. El aviso espera hasta salir de la partida, incluso si abrís el
menú de pausa o la consola. Nunca recarga solo y podés elegir **Más tarde**.

Si hay cambios en GitHub que el anfitrión todavía no instaló, aparece un aviso
para actualizar el servidor con un enlace a esta guía. Recargar la página en ese
caso no instala los cambios del repo.

## Para quien tiene una copia local con Docker

Desde la carpeta del repo:

```sh
python3 scripts/update-local.py
```

Comprueba GitHub sin modificar tus archivos. Código de salida: `0` actualizado,
`1` hay actualización, `2` no se pudo comprobar o el historial requiere revisión.
El chequeo descarga referencias de Git; no instala código ni reinicia Docker.

Para tomar lo último y reconstruir un servidor vacío:

```sh
python3 scripts/update-local.py --apply
```

El comando acepta solamente `origin` apuntando a este repo, rama `master`,
una copia sin cambios locales y una actualización por avance directo de Git
(`--ff-only`). No hace reset, stash ni merges de historiales propios. Comprueba
que no haya jugadores antes de modificar y antes de reiniciar. Conserva
`data/main/`, regenera el paquete privado para la versión nueva, compila cliente,
servidor y gateway, y ejecuta `docker compose up -d`.

La compilación puede demorar. Si falla, los contenedores existentes no se
reinician; corregí el error y repetí `--apply`, que también permite reconstruir
cuando el código ya está actualizado. Guardá una copia de tus datos originales.

Requiere Git, Python 3.9+, Node.js y Docker con Compose. El framework debe estar
en la carpeta hermana `wasm-game-framework`, o en `COD2_WASM_FRAMEWORK_DIR`:

```sh
git clone https://github.com/theodorecharles/wasm-game-framework.git ../wasm-game-framework
```

La compilación fija el framework a la versión 0.9.2. Emscripten se usa desde
Docker si no está instalado localmente. Se necesitan los IWD originales de
CoD2 1.3 en `data/main/`, incluidos los `localized_english_*.iwd`, y el arte
original preparado en `data/browser/web/`. `build-docker.sh` puede extraer el
icono de `COD2_ORIGINAL_ZIP` y el logo de `data/main/iw_09.iwd`.

## Cómo llegan los archivos a los jugadores

El anfitrión prepara los originales una vez, con:

```sh
python3 scripts/prepare-browser-bootstrap.py
```

El paquete privado actual de Toujane ocupa unos **165.6 MB**. Cada jugador lo
descarga por HTTP/HTTPS desde el mismo servidor al entrar y lo conserva en
IndexedDB. El manifiesto verifica tamaño y SHA-256, y una versión de assets nueva
invalida la caché anterior. La caché pertenece a cada navegador y URL: cambiar
de dominio de túnel puede requerir descargarlo otra vez.

No hace falta pasar un ZIP a cada jugador para entrar al servidor que ya está
preparado. Quien quiera levantar un servidor propio debe importar los IWD de
su instalación legítima en `data/main/`; no vienen con el clone ni se publican
en el repo, las imágenes o Releases.

Los mapas, modelos, texturas, sonidos y ejecutables originales son contenido
de Activision. Publicar el código no concede derechos sobre esos archivos.
Una descarga pública de esos archivos requiere los permisos de redistribución
aplicables; no asumimos que poseer una copia autorice compartirla con terceros.
Ver los [términos de software de Activision](https://www.activision.com/legal/software-terms-of-use).
Esta guía describe el mecanismo técnico, no concede una licencia del juego.

## Comprobaciones de versión

`/build-info.json` identifica la revisión y el contenido de la compilación.
`GET /version` devuelve la versión instalada y el estado de comparación con
GitHub. El gateway consulta GitHub como máximo una vez cada cinco minutos por
revisión, comparte la respuesta entre jugadores y no necesita un token. Una
falla de red o límite de GitHub se informa como `unknown`; no anuncia falsamente
que la copia esté actualizada. Historiales adelantados o divergentes no se
anuncian como una actualización normal.

El aviso aparece después de instalar por primera vez una versión que incluya
esta función. Las copias anteriores necesitan actualizarse manualmente una vez.
