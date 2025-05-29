#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "list.h"
#include "logs.h"
#include "properties.h"

#define NAME "set"

static int fn_set(LIST_ptr args) {
  if (list_size(*args) != 3) {
    fprintf(stderr, "Uso: " NAME " [propiedad] [valor]\n");
    return EXIT_FAILURE;
  }

  NODE_ptr node_prop = list_index_get(1, *args);
  if (node_prop == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, NAME, "No se puedo obtener el parametro [prop]");
    return EXIT_FAILURE;
  }
  NODE_ptr node_value = list_index_get(2, *args);
  if (node_value == NULL) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, NAME, "No se puedo obtener el parametro [value]");
    return EXIT_FAILURE;
  }
  char *text_prop = (char *)node_prop->point;
  char *text_value = (char *)node_value->point;
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
  int result_code = prop_reg_type_validate(config_prop->type, text_value);
  switch (result_code) {
  case 0: log_add(NULL, ERROR, NAME, "No se encontro el tipo de dato que contiene la propiedad"); return EXIT_FAILURE;
  case 2:
    log_add(NULL, ERROR, NAME,
            "El formato de la entrada no coincide con el formato "
            "requerido por la propiedad");
    return EXIT_FAILURE;
  }

  (void)(free(config_prop->key), free(config_prop->value));
  prop_struct new_prop = {strdup(text_prop), strdup(text_value), config_prop->type};
  if (!list_index_set(position_prop, prop_register, &new_prop, sizeof(prop_struct))) {
    DynSetLog(NULL);
    log_add(NULL, ERROR, NAME, "No se puedo modificar la propiedad");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

const Command cmd_set = {
    NAME,
    "Modifica una propiedad del programa",
    NULL,
    "Comando: set <nombre> <valor>\n"
    "Descripcion:\n"
    "  Modifica el valor de una propiedad previamente registrada.\n"
    "\n"
    "Uso:\n"
    "  set <nombre> <valor>\n"
    "\n"
    "Argumentos:\n"
    "  <nombre> : Nombre de la propiedad a modificar.\n"
    "  <valor>  : Nuevo valor que se asignara a la propiedad.\n"
    "\n"
    "Notas:\n"
    "  - El comando solo funciona si la propiedad ya existe en el registro.\n"
    "  - El nuevo valor debe cumplir con el formato y tipo de la propiedad.\n"
    "  - Si el tipo no coincide, se muestra un error explicando el fallo.\n"
    "  - Devuelve EXIT_FAILURE si ocurre un error durante la validacion o asignacion.\n"
    "  - El comando requiere exactamente 2 argumentos adicionales.\n",
    {2, 2},
    fn_set,
};
