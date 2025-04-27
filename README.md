# netXenium

**netXenium** es un programa en desarrollo que permite automatizar y operar tareas y ataques de red de alto y bajo nivel. Está diseñado para ser una herramienta flexible tanto para administradores como para investigadores de seguridad.

> **Advertencia:** Este software fue desarrollado con fines educativos y de investigación en entornos controlados o laborales. El desarrollador **no se hace responsable** del uso indebido que se le pueda dar.

## Características actuales

- Shell interactiva con comandos específicos para operaciones de red.
- Interfaz de línea de comandos colorida y dinámica.
- Automatización básica de tareas comunes en redes.
- Comandos internos como `help` y `exit` para facilitar el uso.

## Requisitos

- Sistema operativo **Linux**
- **Permisos de superusuario** (root)
- **libpcap** instalada

## Instalación

### Opción 1: Descargar releases

Puedes descargar binarios precompilados desde la sección [Releases](https://github.com/lenx443/netXenium/releases) del repositorio (si no estan disponible en este momento se debera usar la opción 2 asta que se suban releases).

### Opción 2: Compilación manual

Instala las dependencias necesarias y compila el proyecto:

```sh
pkg update
pkg install clang libpcap cmake make
git clone https://github.com/lenx443/netXenium.git
cd ARP-Spoofer-C
mkdir build && cd build
cmake ..
make
```

Esto generará el binario `xenium` dentro del directorio `build`.

## Uso

Ejecuta el programa con permisos de superusuario:

```sh
sudo ./xenium
```

Dentro de la shell:

1. Usa el comando `help` para ver la ayuda disponible.
2. Utiliza `exit` o `CTRL+C` para salir de la shell.

## Próximamente

- Soporte para **scripting** de comandos para automatizar sesiones completas.
- Mejoras en la **interfaz**:
  - Historial de comandos
  - Autocompletado
  - Cursor interactivo y accesos rápidos
- Nuevos comandos como `input` y `echo` para enriquecer el scripting.
- Mejoras en el **manejo de errores** con mensajes más detallados.

## Autor

**Lenx**\
Proyecto de investigación 2024-2025

---

**¡Recuerda!** netXenium debe ser usado únicamente en redes propias o en entornos de laboratorio controlado.


