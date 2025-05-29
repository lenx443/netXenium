#include <stdlib.h>

#include "commands.h"
#include "list.h"
#include "logs.h"
#include "properties.h"

#define NAME "del"

int fn_del(LIST_ptr args) {
  NODE_ptr node_prop = list_index_get(1, *args);
  char *str_prop = (char *)node_prop->point;
  int index_prop = prop_reg_search_key(str_prop, *prop_register);
  if (index_prop == -1) {
    log_add(NULL, ERROR, NAME, "No se pudo eliminar la propiedad");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  list_erase_at_index(prop_register, index_prop);
  return EXIT_SUCCESS;
}

const Command cmd_del = {
    NAME,
    "Elimina una propiedad",
    "Elimina una propiedad del registro",
    "Comando: del <nombre>\n"
    "Descripcion:\n"
    "  Elimina una propiedad del registro actual.\n"
    "  Si la propiedad no existe, se muestra un mensaje de error.\n"
    "\n"
    "Uso:\n"
    "  del <nombre>  --> Elimina la propiedad con el nombre especificado.\n"
    "\n"
    "Argumentos:\n"
    "  <nombre> : Nombre exacto de la propiedad a eliminar.\n"
    "\n"
    "Notas:\n"
    "  - Si la propiedad no existe en el registro, el comando fallara.\n"
    "  - Devuelve EXIT_FAILURE si no se pudo encontrar la propiedad.\n"
    "  - En caso exitoso, la propiedad se elimina del registro.\n",
    {1, 1},
    fn_del,
};
