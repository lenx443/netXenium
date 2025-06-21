#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "list.h"
#include "logs.h"
#include "properties.h"
#include "properties_types.h"

#define NAME "echo"

static int fn_echo(LIST_ptr args) {
  int args_size = list_size(*args);
  if (args_size == 2) {
    NODE_ptr node_value = list_index_get(1, *args);
    char *str_value = (char *)node_value->point;
    if (is_string(str_value)) {
      char printeable[65536];
      to_string(printeable, str_value);
      fputs(printeable, stdout);
    }
  } else {
    prop_struct *output_prop = prop_reg_value("OUTPUT", *prop_register);
    if (output_prop == NULL) {
      log_clear(NULL);
      return EXIT_SUCCESS;
    }
    if (output_prop->type != STRING) return EXIT_SUCCESS;
    char printeable[65536];
    to_string(printeable, output_prop->value);
    fputs(printeable, stdout);
  }
  return EXIT_SUCCESS;
}

const Command cmd_echo = {
    NAME,
    "Mustra texto en la terminal",
    "Muestra un texto en la terminal (STDOUT) "
    "con el contenido de la propiedad OUTPUT",
    "Comando: echo [valor]\n"
    "Descripcion:\n"
    "  Muestra por pantalla el valor proporcionado o, si no se proporciona, el valor de "
    "la propiedad OUTPUT.\n"
    "\n"
    "Uso:\n"
    "  echo [valor]\n"
    "\n"
    "Argumentos:\n"
    "  [valor] : (Opcional) Cadena a imprimir. Si se omite, se usa el valor de la "
    "propiedad OUTPUT.\n"
    "\n"
    "Notas:\n"
    "  - Si se proporciona un argumento, debe ser una cadena valida.\n"
    "  - Si no se proporciona, se intenta obtener la propiedad OUTPUT del registro.\n"
    "  - Si OUTPUT no existe o no es de tipo STRING, no se imprime nada.\n"
    "  - El valor es formateado antes de imprimirse (por ejemplo, eliminando comillas si "
    "es una cadena).\n",
    {0, 1},
    fn_echo,
};
