#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "commands.h"
#include "list.h"
#include "logs.h"
#include "properties.h"
#include "properties_types.h"

#define NAME "input"

static int fn_input(LIST_ptr args) {
  prop_struct *in_prop = NULL;
  int (*validator)(char *) = NULL;
  int args_len = list_size(*args);
  if (args_len == 2) {
    NODE_ptr node_in = list_index_get(1, *args);
    char *str_in = (char *)node_in->point;
    if ((in_prop = prop_reg_value(str_in, *prop_register)) == NULL) {
      if (!prop_reg_add(prop_register, "str_in", "", STRING)) {
        log_add(NULL, ERROR, NAME, "No se pudo crear la uneva propiedad");
        log_show_and_clear(NULL);
        return EXIT_FAILURE;
      }
      in_prop = prop_reg_value(str_in, *prop_register);
      validator = is_string;
    } else {
      for (int i = 0; map_types[i].key != OTHER; i++) {
        if (map_types[i].key == in_prop->type) {
          validator = map_types[i].validate;
          break;
        }
      }
    }
  } else {
    if ((in_prop = prop_reg_value("INPUT", *prop_register)) == NULL) {
      if (!prop_reg_add(prop_register, "INPUT", "", STRING)) {
        log_add(NULL, ERROR, NAME, "No se pudo crear la nueva propiedad");
        log_show_and_clear(NULL);
        return EXIT_FAILURE;
      }
      in_prop = prop_reg_value("INPUT", *prop_register);
      validator = is_string;
    } else {
      for (int i = 0; map_types[i].key != OTHER; i++) {
        if (map_types[i].key == in_prop->type) {
          validator = map_types[i].validate;
          break;
        }
      }
    }
  }
  char in_buffer[65536];
  fgets(in_buffer, 65536, stdin);
  in_buffer[strcspn(in_buffer, "\n")] = '\0';
  if (validator != NULL) {
    if (!validator(in_buffer)) {
      log_add(NULL, ERROR, NAME, "La entrada no coincide con el valor requerido por la propiedad");
      log_add(NULL, ERROR, NAME, AMARILLO "%s" RESET " es invalido", in_buffer);
      log_show_and_clear(NULL);
      return EXIT_FAILURE;
    }
  }
  free(in_prop->value);
  in_prop->value = strdup(in_buffer);
  return EXIT_SUCCESS;
}

const Command cmd_input = {
    NAME,
    "espera una entrada del usuario",
    "El commando input se urilizar para recivir una entrada del usuario "
    "(stdin)",
    "Comando: input [propiedad]\n"
    "Descripcion:\n"
    "  Lee una línea de entrada estándar (stdin) y la guarda en una propiedad.\n"
    "\n"
    "Uso:\n"
    "  input [propiedad]\n"
    "\n"
    "Argumentos:\n"
    "  [propiedad] : (Opcional) Nombre de la propiedad donde guardar la entrada.\n"
    "                Si no se proporciona, se usa la propiedad \"INPUT\".\n"
    "\n"
    "Funcionamiento:\n"
    "  - Si la propiedad especificada no existe, se crea con tipo STRING.\n"
    "  - Se valida la entrada usando la función asociada al tipo de la propiedad.\n"
    "  - Si la entrada no es válida, se muestra un error y no se modifica la propiedad.\n"
    "  - La entrada se lee hasta un salto de línea y se almacena en la propiedad.\n"
    "\n"
    "Errores comunes:\n"
    "  - Si la entrada no cumple el formato requerido, se muestra un mensaje de error.\n"
    "  - Si no se puede crear la propiedad cuando no existe, se reporta un error.\n",
    {0, 1},
    fn_input,
};
