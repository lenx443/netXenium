#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "commands.h"
#include "config.h"
#include "errs.h"
#include "list.h"

#define NAME "get"

/*
 * Este comando permite obtener el
 * valor de una propiedad.
 */
static int fn_get(LIST_ptr args) {
  /*
   * Se verifica que los argumentos sean
   * validos.
   */
  if (list_size(*args) != 2) {
    fprintf(stderr, "Uso: " NAME " [propiedad|all]\n");
    return EXIT_FAILURE;
  }
  NODE_ptr node_prop = list_index_get(1, *args);
  CHECK_ERROR_RETURN(node_prop == NULL,
                     "No se puedo obtener el parametro [prop]", EXIT_FAILURE);

  /*
   * Si el argumeto es `all` se muestran
   * los valores de todas las propiedades.
   */
  char *text_prop = (char *)node_prop->point;
  if (strcmp(text_prop, "all") == 0) {
    NODE_ptr iterator = NULL;
    FOR_EACH(&iterator, *config) {
      config_struct *current_cfg = (config_struct *)iterator->point;
      if (current_cfg->type != OTHER) {
        printf(AZUL "%s" RESET " = " AMARILLO "%s" RESET "\n", current_cfg->key,
               current_cfg->value);
      }
    }
  } else {
    /*
     * Se obtiene el valor de la propiedad
     * pasada por argumeto.
     */
    int position_prop = config_search_key(text_prop, *config);
    CHECK_ERROR_RETURN(position_prop == -1, "No se encontro la propiedad",
                       EXIT_FAILURE);
    NODE_ptr value_prop = list_index_get(position_prop, *config);
    CHECK_ERROR_RETURN(value_prop == NULL,
                       "No se pudo obtener el valor de la propiedad",
                       EXIT_FAILURE);
    config_struct *config_prop = (config_struct *)value_prop->point;
    printf("%s\n", config_prop->value);
  }
  return EXIT_SUCCESS;
}

// Se define la estructura del comando.
const Command cmd_get = {
    NAME,
    "obtiene el valor de una propiedad del programa",
    fn_get,
};
