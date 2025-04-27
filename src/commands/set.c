#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "config.h"
#include "errs.h"
#include "list.h"

#define NAME "set"

/*
 * Este comando permite modificar
 * el valor de una propiedad.
 */
static int fn_set(LIST_ptr args) {
  /*
   * Se verifica que los argumentos
   * sean validos.
   */
  if (list_size(*args) != 3) {
    fprintf(stderr, "Uso: " NAME " [propiedad] [valor]\n");
    return EXIT_FAILURE;
  }

  /*
   * Se obtienen los datos necesarios
   * para modificar la propiedad.
   */
  NODE_ptr node_prop = list_index_get(1, *args);
  CHECK_ERROR_RETURN(node_prop == NULL,
                     "No se puedo obtener el parametro [prop]", EXIT_FAILURE);
  NODE_ptr node_value = list_index_get(2, *args);
  CHECK_ERROR_RETURN(node_value == NULL,
                     "No se puedo obtener el parametro [value]", EXIT_FAILURE);
  char *text_prop = (char *)node_prop->point;
  char *text_value = (char *)node_value->point;
  int position_prop = config_search_key(text_prop, *config);
  CHECK_ERROR_RETURN(position_prop == -1, "No se encontro la propiedad",
                     EXIT_FAILURE);
  NODE_ptr value_prop = list_index_get(position_prop, *config);
  CHECK_ERROR_RETURN(value_prop == NULL,
                     "No se pudo obtener el valor de la propiedad",
                     EXIT_FAILURE);

  /*
   * Se Valida que el nuevo valor
   * cumpla con el formato requerido
   * por la propiedad.
   */
  config_struct *config_prop = (config_struct *)value_prop->point;
  int result_code = config_type_validate(config_prop->type, text_value);
  switch (result_code) {
  case 0:
    print_error(NAME,
                "No se encontro el tipo de dato que contiene la propiedad");
    return EXIT_FAILURE;
  case 2:
    print_error(NAME, "El formato de la entrada no coincide con el formato "
                      "requerido por la propiedad");
    return EXIT_FAILURE;
  }

  /*
   * Se modifica el valor de la
   * propiedad.
   */
  (void)(free(config_prop->key), free(config_prop->value));
  config_struct new_prop = {strdup(text_prop), strdup(text_value),
                            config_prop->type};
  CHECK_ERROR_RETURN(
      !list_index_set(position_prop, config, &new_prop, sizeof(config_struct)),
      "No se puedo modificar la propiedad", EXIT_FAILURE);
  return EXIT_SUCCESS;
}

// Se crea la estructura del comando.
const Command cmd_set = {
    NAME,
    "modifica una propiedad del programa",
    fn_set,
};
