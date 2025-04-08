# ARP Spoofer

Este es un proyecto en desarrollo llamado **ARP Spoofer**, una herramienta escrita en C que permite realizar ataques de spoofing ARP (Address Resolution Protocol). Esta técnica se utiliza para interceptar el tráfico de red entre un objetivo y su puerta de enlace (router), haciéndoles creer que tú eres el otro dispositivo.

> **Advertencia:** Este software tiene propósitos educativos y de investigación en entornos controlados. El uso indebido puede ser ilegal.

## Características actuales

- Descubre la dirección IP y MAC del objetivo y del router.
- Envía paquetes ARP falsos para envenenar las tablas ARP.
- Interfaz interactiva desde terminal (selección de interfaz de red, IPs, etc).
- Interfaz colorida para mejorar la experiencia visual.
- Automatiza el proceso de envío y restauración de ARP.

## Compilación

Usa CMake para compilar el proyecto:

```sh
mkdir build && cd build
cmake ..
make
```

Esto generará el binario `arp` dentro del directorio `build`.

## Uso

```sh
sudo ./arp
```

1. Introduce la interfaz de red (por ejemplo `eth0` o `wlan0`).
2. Introduce la IP del objetivo.
3. Introduce la IP del router.
4. El programa obtendrá automáticamente las MACs y lanzará el ataque.
5. Presiona `Q` para detener el ataque y restaurar las tablas ARP.

## Dependencias

- Linux
- Compilador C compatible con POSIX
- Permisos de superusuario (uso de sockets RAW)
- Pcap instalado

## Autor

**Lenx**\
Proyecto de investigación 2024

---

**¡Recuerda!** Este proyecto debe ejecutarse *solo* con fines de aprendizaje en redes propias o laboratorios controlados.


