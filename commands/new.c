#include <ctype.h>
#include <stdlib.h>

#include "colors.h"
#include "commands.h"
#include "list.h"
#include "logs.h"
#include "properties.h"

#define NAME "new"

static int fn_new(LIST_ptr args) {
  NODE_ptr node_name = list_index_get(1, *args);
  NODE_ptr node_type = list_index_get(2, *args);
  char *prop_name = (char *)node_name->point;
  char *prop_type = (char *)node_type->point;

  for (int i = 0; prop_name[i] != '\0'; i++) {
    char c = toupper(prop_name[i]);
    if (!isalnum(prop_name[i]) && c != '_') {
      log_add(NULL, ERROR, NAME, "El nombre {" AZUL "\"%s\"" RESET "} no es adecuado para una propiedad", prop_name);
      return 153;
    }
  }
  if (prop_reg_search_key(prop_name, *prop_register) != -1) {
    log_add(NULL, ERROR, NAME, "El nombre {" AZUL "\"%s\"" RESET "} ya fue usado por otra propiedad", prop_name);
    return 153;
  } else
    log_clear(NULL);
  log_clear(NULL);
  prop_types type_value = OTHER;
  int (*validate_type)(char *);
  for (int i = 0; map_types[i].key != OTHER; i++) {
    if (strcmp(map_types[i].key_str, prop_type) == 0) {
      type_value = map_types[i].key;
      validate_type = map_types[i].validate;
      break;
    }
  }
  if (type_value == OTHER) {
    log_add(NULL, ERROR, NAME, "El tipo {" VERDE "\"%s\"" RESET "} no es valido", prop_type);
    return 153;
  }
  char *prop_value = NULL;
  int args_len = list_size(*args);
  if (args_len == 4) {
    NODE_ptr node_value = list_index_get(3, *args);
    prop_value = (char *)node_value->point;

    if (!validate_type(prop_value)) {
      log_add(NULL, ERROR, NAME,
              "El valor {" AMARILLO "\"%s\"" RESET "} no es valido para propiedades del tipo " VERDE "%s" RESET,
              prop_value, prop_type);
      return 153;
    }
  }
  if (!prop_reg_add(prop_register, prop_name, prop_value ? prop_value : "", type_value)) {
    log_add(NULL, ERROR, NAME, "Ocurrio un error al agregar la nueva propiedad al registro");
    log_show_and_clear(NULL);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

const Command cmd_new = {
    NAME,
    "Crea una nueva propiedad",
    "Permite crear una nueva propiedad nueva en el programa",
    "Comando: new <nombre> <tipo> [valor]\n"
    "Descripcion:\n"
    "  Crea una nueva propiedad en el registro actual.\n"
    "  La propiedad tendra un nombre, un tipo y, opcionalmente, un valor inicial.\n"
    "\n"
    "Uso:\n"
    "  new <nombre> <tipo>         --> Crea la propiedad con valor por defecto ('').\n"
    "  new <nombre> <tipo> <valor> --> Crea la propiedad con el valor especificado.\n"
    "\n"
    "Argumentos:\n"
    "  <nombre> : Nombre de la propiedad. Debe ser alfanumerico o contener '_'.\n"
    "  <tipo>   : Tipo de la propiedad. Debe ser uno de los tipos reconocidos.\n"
    "  <valor>  : (opcional) Valor inicial de la propiedad. Debe ser valido para el tipo.\n"
    "\n"
    "Tipos disponibles:\n"
    "  (Consultar la lista definida en map_types; por ejemplo: int, str, bool, etc.)\n"
    "\n"
    "Notas:\n"
    "  - El nombre no puede contener caracteres especiales ni estar repetido.\n"
    "  - Si el tipo es invalido o el valor no es compatible, se muestra un error.\n"
    "  - Si no se especifica valor, se asigna una cadena vacia por defecto.\n"
    "  - Devuelve 153 en caso de error de validacion o si el nombre ya existe.\n"

    ,
    {2, 3},
    fn_new,
};
