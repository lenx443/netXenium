#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "commands.h"
#include "functions.h"
#include "list.h"
#include "logs.h"
#include "properties.h"
#include "properties_types.h"

#define NAME "iface"

static int fn_iface(LIST_ptr args) {
  int args_size = list_size(*args);
  NODE_ptr node_option = list_index_get(1, *args);
  char *option = (char *)node_option->point;

  if (strcmp(option, "change") == 0) {
    if (args_size < 4) {
      log_add(NULL, ERROR, NAME, "Los datos son invalidos");
      return 153;
    }
    NODE_ptr node_addr = list_index_get(2, *args);
    NODE_ptr node_iface = list_index_get(3, *args);
    char *addr = (char *)node_addr->point;
    char *iface = (char *)node_iface->point;

    for (int i = 0; i < strlen(addr); i++)
      addr[i] = tolower(addr[i]);
    for (int i = 0; i < strlen(iface); i++)
      iface[i] = tolower(iface[i]);

    if (strcmp(addr, "addr") == 0) {
      prop_struct *new_addr_prop = prop_reg_value("CHG_ADDR", *prop_register, 1);
      if (new_addr_prop == NULL) {
        log_add(NULL, ERROR, NAME,
                "No se pudo acceder a la propiedad " AZUL "CHG_ADDR" RESET);
        log_show_and_clear(NULL);
        return EXIT_FAILURE;
      }
      if (new_addr_prop->type != IP) {
        log_add(NULL, ERROR, NAME,
                "El valor de la propiedad " AZUL "CHG_ADDR" RESET "no es una IP");
        return 153;
      }
      uint32_t new_addr;
      if (!to_ip(&new_addr, new_addr_prop->value)) {
        log_add(NULL, ERROR, NAME,
                "Ocurrio un error al combertir la IP a binario (big-endian)");
        log_show_and_clear(NULL);
        return 0;
      }
      if (!set_local_ip(new_addr, iface)) {
        log_add(NULL, ERROR, NAME,
                "Ocurrio un error al cambiar la IP en la interfaz " CIAN "%s" RESET,
                iface);
        log_show_and_clear(NULL);
        return 0;
      }
    } else if (strcmp(addr, "netmask") == 0) {
      prop_struct *new_addr_prop = prop_reg_value("CHG_NETMASK", *prop_register, 1);
      if (new_addr_prop == NULL) {
        log_add(NULL, ERROR, NAME,
                "No se pudo acceder a la propiedad " AZUL "CHG_NETMASK" RESET);
        log_show_and_clear(NULL);
        return EXIT_FAILURE;
      }
      if (new_addr_prop->type != IP) {
        log_add(NULL, ERROR, NAME,
                "El valor de la propiedad " AZUL "CHG_NETMASK" RESET "no es una IP");
        return 153;
      }
      uint32_t new_addr;
      if (!to_ip(&new_addr, new_addr_prop->value)) {
        log_add(NULL, ERROR, NAME,
                "Ocurrio un error al combertir la mascara de red a binario "
                "(big-endian)");
        log_show_and_clear(NULL);
        return 0;
      }
      if (!set_local_netmask(new_addr, iface)) {
        log_add(NULL, ERROR, NAME,
                "Ocurrio un error al cambiar la mascara de red en la interfaz " CIAN
                "%s" RESET,
                iface);
        log_show_and_clear(NULL);
        return 0;
      }
    } else if (strcmp(addr, "broad") == 0) {
      prop_struct *new_addr_prop = prop_reg_value("CHG_BROADCAST", *prop_register, 1);
      if (new_addr_prop == NULL) {
        log_add(NULL, ERROR, NAME,
                "No se pudo acceder a la propiedad " AZUL "CHG_BROADCAST" RESET);
        log_show_and_clear(NULL);
        return EXIT_FAILURE;
      }
      if (new_addr_prop->type != IP) {
        log_add(NULL, ERROR, NAME,
                "El valor de la propiedad " AZUL "CHG_BROADCAST" RESET "no es una IP");
        return 153;
      }
      uint32_t new_addr;
      if (!to_ip(&new_addr, new_addr_prop->value)) {
        log_add(NULL, ERROR, NAME,
                "Ocurrio un error al combertir el BROADCAST a binario "
                "(big-endian)");
        log_show_and_clear(NULL);
        return 0;
      }
      if (!set_local_broadcast(new_addr, iface)) {
        log_add(NULL, ERROR, NAME,
                "Ocurrio un error al cambiar el BROADCAST en la interfaz " CIAN
                "%s" RESET,
                iface);
        log_show_and_clear(NULL);
        return 0;
      }
    } else if (strcmp(addr, "hwaddr") == 0) {
      prop_struct *new_addr_prop = prop_reg_value("CHG_HWADDR", *prop_register, 1);
      if (new_addr_prop == NULL) {
        log_add(NULL, ERROR, NAME,
                "No se pudo acceder a la propiedad " AZUL "CHG_HWADDR" RESET);
        log_show_and_clear(NULL);
        return EXIT_FAILURE;
      }
      if (new_addr_prop->type != MAC) {
        log_add(NULL, ERROR, NAME,
                "El valor de la propiedad " AZUL "CHG_HWADDR" RESET "no es una MAC");
        return 153;
      }
      uint8_t new_addr[6];
      if (!to_mac(&new_addr, new_addr_prop->value)) {
        log_add(NULL, ERROR, NAME, "Ocurrio un error al combertir la MAC a bytes");
        log_show_and_clear(NULL);
        return 0;
      }
      if (!set_local_mac(new_addr, iface)) {
        log_add(NULL, ERROR, NAME,
                "Ocurrio un error al cambiar la MAC en la interfaz " CIAN "%s" RESET,
                iface);
        log_show_and_clear(NULL);
        return 0;
      }
    }
  } else if (strcmp(option, "get") == 0) {
    if (args_size < 3) {
      log_add(NULL, ERROR, NAME, "Los datos son invalidos");
      return 153;
    }
    NODE_ptr node_addr = list_index_get(2, *args);
    char *addr = (char *)node_addr->point;
    for (int i = 0; i < strlen(addr); i++)
      addr[i] = tolower(addr[i]);

    if (args_size == 3 && strcmp(addr, "ifaces") == 0) {
      LIST_ptr ifaces = get_ifaces_list();
      if (!ifaces) {
        log_add(NULL, ERROR, NAME, "No se pudieron alistar las interfaces de red");
        log_show_and_clear(NULL);
        return EXIT_FAILURE;
      }
      NODE_ptr iface_node = NULL;
      FOR_EACH(&iface_node, *ifaces) {
        char *iface = (char *)iface_node->point;

        uint32_t addr;
        uint32_t netmask;
        uint32_t broadcast;
        uint8_t hwaddr[6];

        char addr_str[16];
        char netmask_str[16];
        char broadcast_str[16];
        char hwaddr_str[18];

        int gli = get_local_ip(&addr, iface);
        int gln = get_local_netmask(&netmask, iface);
        int glb = get_local_broadcast(&broadcast, iface);
        int glm = get_local_mac(hwaddr, iface);

        if (gli) from_ip(addr_str, &addr, sizeof(uint32_t));
        if (gln) from_ip(netmask_str, &netmask, sizeof(uint32_t));
        if (glb) from_ip(broadcast_str, &broadcast, sizeof(uint32_t));
        if (glm) from_mac(hwaddr_str, hwaddr, 18);

        printf("iface: %s\n", iface);
        if (gli || gln || glb || glm) {
          if (gli) printf("    addr: %s\n", addr_str);
          if (gln) printf("    netmask: %s\n", netmask_str);
          if (glb) printf("    broadcast: %s\n", broadcast_str);
          if (glm) printf("    hwaddr: %s\n", hwaddr_str);
        }
      }
      log_clear(NULL);
    } else {
      if (args_size != 4) {
        log_add(NULL, ERROR, NAME, "Los datos son invalidos");
        return 153;
      }
      NODE_ptr node_iface = list_index_get(3, *args);
      char *iface = (char *)node_iface->point;
      for (int i = 0; i < strlen(iface); i++)
        iface[i] = tolower(iface[i]);

      if (strcmp(addr, "addr") == 0) {
        prop_struct *rslt_addr_prop = NULL;
        rslt_addr_prop = prop_reg_value("RSLT_ADDR", *prop_register, 0);
        if (rslt_addr_prop == NULL) {
          if (!prop_reg_add(prop_register, "RSLT_ADDR", "", IP)) {
            log_clear(NULL);
            log_add(NULL, ERROR, NAME, "No se pudo crear la propiedad RSLT_ADDR");
            log_show_and_clear(NULL);
            return EXIT_FAILURE;
          }
          rslt_addr_prop = prop_reg_value("RSLT_ADDR", *prop_register, 0);
        } else {
          if (rslt_addr_prop->type != IP) {
            log_add(NULL, ERROR, NAME, "La propiedad RSLT_ADDR no es una IP");
            log_show_and_clear(NULL);
            return EXIT_FAILURE;
          }
        }
        uint32_t local_addr;
        if (!get_local_ip(&local_addr, iface)) {
          log_add(NULL, ERROR, NAME, "No se pudo obtener la direccion IP");
          log_add(NULL, ERROR, NAME, CIAN "{%s}" RESET " <- interfaz", iface);
          log_show_and_clear(NULL);
          return EXIT_FAILURE;
        }
        char local_addr_str[16];
        from_ip(local_addr_str, &local_addr, 16);
        free(rslt_addr_prop->value);
        rslt_addr_prop->value = strdup(local_addr_str);
      }
      if (strcmp(addr, "netmask") == 0) {
        prop_struct *rslt_addr_prop = NULL;
        rslt_addr_prop = prop_reg_value("RSLT_NETMASK", *prop_register, 0);
        if (rslt_addr_prop == NULL) {
          if (!prop_reg_add(prop_register, "RSLT_NETMASK", "", IP)) {
            log_add(NULL, ERROR, NAME, "No se pudo crear la propiedad RSLT_NETMASK");
            log_show_and_clear(NULL);
            return EXIT_FAILURE;
          }
          rslt_addr_prop = prop_reg_value("RSLT_NETMASK", *prop_register, 0);
        } else {
          if (rslt_addr_prop->type != IP) {
            log_add(NULL, ERROR, NAME, "La propiedad RSLT_NETMASK no es una IP");
            log_show_and_clear(NULL);
            return EXIT_FAILURE;
          }
        }
        uint32_t local_addr;
        if (!get_local_netmask(&local_addr, iface)) {
          log_add(NULL, ERROR, NAME, "No se pudo obtener la direccion mascara de red");
          log_add(NULL, ERROR, NAME, CIAN "{%s}" RESET " <- interfaz", iface);
          log_show_and_clear(NULL);
          return EXIT_FAILURE;
        }
        char local_addr_str[16];
        from_ip(local_addr_str, &local_addr, 16);
        free(rslt_addr_prop->value);
        rslt_addr_prop->value = strdup(local_addr_str);
      }
      if (strcmp(addr, "broad") == 0) {
        prop_struct *rslt_addr_prop = NULL;
        rslt_addr_prop = prop_reg_value("RSLT_BROADCAST", *prop_register, 0);
        if (rslt_addr_prop == NULL) {
          if (!prop_reg_add(prop_register, "RSLT_BROADCAST", "", IP)) {
            log_add(NULL, ERROR, NAME, "No se pudo crear la propiedad RSLT_BROADCAST");
            log_show_and_clear(NULL);
            return EXIT_FAILURE;
          }
          rslt_addr_prop = prop_reg_value("RSLT_BROADCAST", *prop_register, 0);
        } else {
          if (rslt_addr_prop->type != IP) {
            log_add(NULL, ERROR, NAME, "La propiedad RSLT_BROADCAST no es una IP");
            log_show_and_clear(NULL);
            return EXIT_FAILURE;
          }
        }
        uint32_t local_addr;
        if (!get_local_broadcast(&local_addr, iface)) {
          log_add(NULL, ERROR, NAME, "No se pudo obtener el broadcast");
          log_add(NULL, ERROR, NAME, CIAN "{%s}" RESET " <- interfaz", iface);
          log_show_and_clear(NULL);
          return EXIT_FAILURE;
        }
        char local_addr_str[16];
        from_ip(local_addr_str, &local_addr, 16);
        free(rslt_addr_prop->value);
        rslt_addr_prop->value = strdup(local_addr_str);
      }
      if (strcmp(addr, "hwaddr") == 0) {
        prop_struct *rslt_addr_prop = NULL;
        rslt_addr_prop = prop_reg_value("RSLT_HWADDR", *prop_register, 0);
        if (rslt_addr_prop == NULL) {
          if (!prop_reg_add(prop_register, "RSLT_HWADDR", "", MAC)) {
            log_add(NULL, ERROR, NAME, "No se pudo crear la propiedad RSLT_HWADDR");
            log_show_and_clear(NULL);
            return EXIT_FAILURE;
          }
          rslt_addr_prop = prop_reg_value("RSLT_HWADDR", *prop_register, 0);
        } else {
          if (rslt_addr_prop->type != MAC) {
            log_add(NULL, ERROR, NAME, "La propiedad RSLT_HWADDR no es una MAC");
            log_show_and_clear(NULL);
            return EXIT_FAILURE;
          }
        }
        uint8_t local_addr[6];
        if (!get_local_mac(local_addr, iface)) {
          log_add(NULL, ERROR, NAME, "No se pudo obtener la MAC");
          log_add(NULL, ERROR, NAME, CIAN "{%s}" RESET " <- interfaz", iface);
          log_show_and_clear(NULL);
          return EXIT_FAILURE;
        }
        char local_addr_str[18];
        from_mac(local_addr_str, &local_addr, 18);
        free(rslt_addr_prop->value);
        rslt_addr_prop->value = strdup(local_addr_str);
      }
    }
  } else
    return 153;
  return EXIT_SUCCESS;
}

const Command cmd_iface = {
    NAME,
    "interactua con interfaces de red",
    "Commando que permite interactuar con Interfaces de red",
    "Uso: iface <accion> <dato> [interfaz]\n"
    "\n"
    "Acciones disponibles:\n"
    "  change <tipo> <interfaz>\n"
    "    Cambia un parámetro de red de una interfaz específica.\n"
    "    Tipos disponibles:\n"
    "      addr      -> Cambia la dirección IP local con el valor de CHG_ADDR\n"
    "      netmask   -> Cambia la máscara de red con el valor de CHG_NETMASK\n"
    "      broad     -> Cambia la dirección de broadcast con el valor de CHG_BROADCAST\n"
    "      hwaddr    -> Cambia la dirección MAC con el valor de CHG_HWADDR\n"
    "\n"
    "    Notas:\n"
    "      - Los valores se toman desde propiedades previamente registradas.\n"
    "      - Las propiedades deben tener el tipo correcto (IP o MAC).\n"
    "\n"
    "  get <tipo> [interfaz]\n"
    "    Obtiene parámetros de red de una interfaz específica o lista todas las "
    "interfaces.\n"
    "    Tipos disponibles:\n"
    "      ifaces    -> Lista todas las interfaces disponibles\n"
    "      addr      -> Obtiene la dirección IP de la interfaz\n"
    "      netmask   -> Obtiene la máscara de red de la interfaz\n"
    "      broad     -> Obtiene la dirección de broadcast de la interfaz\n"
    "      hwaddr    -> Obtiene la dirección MAC de la interfaz\n"
    "\n"
    "    Notas:\n"
    "      - Los valores obtenidos se almacenan en propiedades del tipo:\n"
    "          RSLT_ADDR, RSLT_NETMASK, RSLT_BROADCAST, RSLT_HWADDR\n"
    "      - Si una propiedad no existe, se crea automáticamente.\n"
    "\n"
    "Errores posibles:\n"
    "  - Datos inválidos o insuficientes\n"
    "  - Error al convertir valores IP o MAC\n"
    "  - Interfaz inexistente o inaccesible\n"
    "  - Propiedad no válida o de tipo incorrecto\n"
    "\n"
    "Ejemplos:\n"
    "  iface change addr eth0       -> Cambia la IP de eth0 usando CHG_ADDR\n"
    "  iface get addr eth0          -> Guarda la IP de eth0 en RSLT_ADDR\n"
    "  iface get ifaces             -> Lista todas las interfaces disponibles\n",
    {2, 3},
    fn_iface,
};
