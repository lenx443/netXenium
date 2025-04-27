#include "error_codes.h"
#include "errs.h"

char *strerrs(int code) {
  switch (code) {
  case ERROR_SOCKET:
    return "No se pudo crear el socket";
  case ERROR_SOCKET_FD_LIMIT:
    return "No se pudo crear el socket debido a que se alcanzó el límite de "
           "descriptores de archivo";
  case ERROR_SOCKET_PERMISSIONS:
    return "No se pudo crear el socket debido a que no tienes permiso";

  case ERROR_SOCKET_RAW:
    return "No se pudo crear el socket crudo";
  case ERROR_SOCKET_RAW_FD_LIMIT:
    return "No se pudo crear el socket crudo debido a que se alcanzó el límite "
           "de descriptores de archivo";
  case ERROR_SOCKET_RAW_PERMISSIONS:
    return "No se pudo crear el socket crudo debido a que no tienes permiso";

  case ERROR_IOCTL:
    return "No se pudo usar el ioctl";
  case ERROR_IOCTL_FD_INVALID:
    return "No se pudo usar el ioctl debido a que el descriptor de archivo "
           "utilizado es inválido";
  case ERROR_IOCTL_PERMISSIONS:
    return "No se pudo usar el ioctl debido a que no tienes suficientes "
           "permisos";
  case ERROR_IOCTL_MEMORY:
    return "No se pudo usar el ioctl debido a que no tienes suficiente memoria";

  case ERROR_SEND:
    return "Error al enviar el paquete";
  case ERROR_SEND_BUFFER:
    return "No se pudo enviar el paquete debido a un error en el buffer del "
           "sistema";
  case ERROR_SEND_INTERRUPT:
    return "No se pudo enviar el paquete debido a que una señal interrumpió la "
           "llamada";
  case ERROR_SEND_REFUSED:
    return "No se pudo enviar el paquete debido a que la red o el host es "
           "inalcanzable";
  case ERROR_SEND_CONNECT:
    return "No se pudo enviar el paquete debido a que el otro lado cerró la "
           "conexión abruptamente";
  case ERROR_SEND_TIMEOUT:
    return "No se pudo enviar el paquete debido a que se tardó más tiempo del "
           "esperado en enviarse";
  case ERROR_SEND_ACCESS:
    return "No se pudo enviar el paquete debido a que faltan permisos";

  case ERROR_PCAP:
    return "Error en la librería pcap";
  case ERROR_PCAP_OPEN:
    return "Error al abrir pcap";
  case ERROR_PCAP_COMPILE_FILTER:
    return "Error al compilar un filtro con pcap";
  case ERROR_PCAP_FILTER:
    return "Error al aplicar un filtro en pcap";
  case ERROR_PCAP_PACKET:
    return "Error al recibir un paquete con pcap";

  case ERROR_LIST_MEMORY:
    return "No hay memoria suficiente para realizar la operación";
  case ERROR_LIST_NULL:
    return "La lista es nula o está vacía";
  case ERROR_LIST_NODE_NULL:
    return "El nodo es nulo";
  case ERROR_LIST_FINDED:
    return "El elemento no se encontró en la lista";
  case ERROR_LIST_INDEX:
    return "El índice proporcionado está fuera del rango de la lista";

  case ERROR_TYPE_IP:
    return "El dato no es una dirección IP";
  case ERROR_TYPE_MAC:
    return "El dato no es una dirección MAC";
  case ERROR_TYPE_IFACE:
    return "El dato no es una interfaz de red válida";

  case ERROR_PROP_IP:
    return "La propiedad no es del tipo IP";
  case ERROR_PROP_MAC:
    return "La propiedad no es del tipo MAC";
  case ERROR_PROP_IFACE:
    return "La propiedad no es del tipo IFACE";

  default:
    return "Error no identificado";
  }
}

int code_error = 0;
