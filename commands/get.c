#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "commands.h"
#include "list.h"
#include "logs.h"
#include "properties.h"

#define NAME "get"

static int fn_get(LIST_ptr args) {
  if (list_size(*args) != 2) {
    fprintf(stderr, "Uso: " NAME " [propiedad|props]\n");
    return EXIT_FAILURE;
  }
  NODE_ptr node_prop = list_index_get(1, *args);
  if (node_prop == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, NAME, "No se puedo obtener el parametro [prop]");
    return EXIT_FAILURE;
  }

  char *text_prop = (char *)node_prop->point;
  if (strcmp(text_prop, "props") == 0) {
    int max_key_size = 0;

    NODE_ptr iterator = NULL;
    FOR_EACH(&iterator, *prop_register) {
      prop_struct *current_prop = (prop_struct *)iterator->point;

      int new_key_size = strlen(current_prop->key);
      if (max_key_size < new_key_size) max_key_size = new_key_size;
    }

    iterator = NULL;
    FOR_EACH(&iterator, *prop_register) {
      prop_struct *current_prop = (prop_struct *)iterator->point;
      char *type;
      for (int i = 0; map_types[i].key != OTHER; i++) {
        if (map_types[i].key == current_prop->type) type = strdup(map_types[i].key_str);
      }
      if (current_prop->type != OTHER) {
        printf(AZUL "%-*s" RESET " = " ROJO "%s(" AMARILLO "%s" ROJO ")" RESET "\n", max_key_size, current_prop->key,
               type, current_prop->value);
      }
      free(type);
    }
  } else {
    int position_prop = prop_reg_search_key(text_prop, *prop_register);
    if (position_prop == -1) {
      log_add(NULL, ERROR, NAME, "No se encontro la propiedad");
      return EXIT_FAILURE;
    }
    NODE_ptr value_prop = list_index_get(position_prop, *prop_register);
    if (value_prop == NULL) {
      DynSetLog(NULL);
      log_add(NULL, ERROR, NAME, "No se pudo obtener el valor de la propiedad");
      return EXIT_FAILURE;
    }
    prop_struct *config_prop = (prop_struct *)value_prop->point;
    printf("%s\n", config_prop->value);
  }
  return EXIT_SUCCESS;
}

const Command cmd_get = {
    NAME,
    "Obtiene valor de propiedad",
    "Obtiene el valor de una propiedad del programa",
    "Comando: get <nombre>\n"
    "Descripcion:\n"
    "  Muestra el valor de una propiedad registrada o lista todas las propiedades.\n"
    "\n"
    "Uso:\n"
    "  get <nombre>  --> Muestra el valor de la propiedad especificada.\n"
    "  get props     --> Muestra todas las propiedades registradas con su tipo y valor.\n"
    "\n"
    "Argumentos:\n"
    "  <nombre> : Nombre de la propiedad cuyo valor se desea obtener.\n"
    "  props    : Palabra clave para mostrar todas las propiedades.\n"
    "\n"
    "Notas:\n"
    "  - Si se pasa 'props', se mostraran todas las propiedades registradas.\n"
    "  - Si la propiedad no existe, se mostrara un mensaje de error.\n"
    "  - El comando falla si no se recibe exactamente un argumento adicional.\n"
    "  - Devuelve EXIT_FAILURE si ocurre un error durante la busqueda o acceso.\n"
    "  - Los valores se imprimen en formato: nombre = tipo(valor)\n",
    {1, 1},
    fn_get,
};
