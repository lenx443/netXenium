# netXenium

**netXenium** es un programa en desarrollo que permite automatizar y operar tareas y ataques de red de alto y bajo nivel. Está diseñado para ser una herramienta flexible tanto para administradores como para investigadores de seguridad.

> **Advertencia:** Este software fue desarrollado con fines educativos y de investigación en entornos controlados o laborales. El desarrollador **no se hace responsable** del uso indebido que se le pueda dar.

## Características actuales

- Shell interactiva con comandos específicos para operaciones de red.
  - Soporte para navegacaión con flechas.
  - Historial empleable (almacenamiento local en memoria y en archivo externo)
  - Autocompletado interactivo con soporte para comandos y propiedades.
  - Soporte para caracteres con codificación UTF-8.
- Interfaz de línea de comandos colorida y dinámica.
- Automatización básica de tareas comunes en redes.
- Scripting reutilizable para automatizar tareas repetitivas.
- Comandos internos como `help` y `exit` para facilitar el uso.
- Manejo detallado de errores que emplea sistemas de logging para captar problemas internos

## Requisitos

- Sistema operativo **Linux**
- Entorno Posix (glibc requerido)
- **Permisos de superusuario** (root)
- **libpcap** y **json-c** instaladas

## Instalación

### Opción 1: Descargar releases

Puedes descargar binarios precompilados desde la sección [Releases](https://github.com/lenx443/netXenium/releases) del repositorio (si no están disponible en este momento se deberá usar la opción 2 hasta que se suban `releases`).

### Opción 2: Compilación manual

Instala las dependencias necesarias y compila el proyecto:

```sh
pkg update
pkg install clang libpcap json-c cmake make
git clone https://github.com/lenx443/netXenium.git
cd netXenium
mkdir build && cd build
cmake ..
make
```

Esto generará el binario `ntxenium` dentro del directorio `build`.

## Uso

Ejecuta el programa con permisos de superusuario:

```sh
sudo ./ntxenium
```

Dentro de la shell:

1. Usa el comando `help` para ver la ayuda disponible.
2. Usa el comando `help <cmd>` para ver una descripcion detallada
3. Utiliza `exit` o `CTRL+C` para salir de la shell.

## Autor

**Lenx**\
Proyecto de investigación 2024-2025
<p>(C) 2024-2025 Lenx443 <menasalejandro153@gmail.com></p>

## Contacto y Notas de Uso

¿Has encontrado un error, tienes alguna sugerencia o necesitas ayuda?

- Puedes [abrir un issue](https://github.com/lenx443/netXenium/issues) para reportar problemas o proponer mejoras.
- También puedes ponerte en contacto conmigo a través de [mi perfil de GitHub](https://github.com/lenx443), donde encontrarás más información y enlaces a mis redes sociales.

---

**¡Recuerda!** netXenium debe ser usado únicamente en redes propias o en entornos de laboratorio controlado.

