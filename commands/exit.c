#include <stdlib.h>

#include "commands.h"
#include "list.h"
#include "logs.h"
#include "program.h"

#define NAME "exit"

static int fn_exit(LIST_ptr args) {
  program.closed = 1;
  if (list_size(*args) == 2) {
    NODE_ptr node_status = list_index_get(1, *args);
    char *str_status = (char *)node_status->point;

    char *endptr;
    long exit_status = strtol(str_status, &endptr, 10);
    if (endptr == str_status) {
      log_add(NULL, ERROR, NAME, "'%s' no es un codigo de salida valido");
      return 153;
    }
    program.exit_code = exit_status;
    return exit_status;
  }
  return EXIT_SUCCESS;
}

const Command cmd_exit = {
    NAME,
    "Finaliza el proceso",
    "Finaliza todo el proceso, utiliza su argumento como codigo de salida",
    "Comando: exit [codigo]\n"
    "Descripcion:\n"
    "  Termina la ejecucion del programa actual.\n"
    "  Si se proporciona un codigo numerico, este se usara como\n"
    "  codigo de salida del programa.\n"
    "\n"
    "Uso:\n"
    "  exit           --> Finaliza el programa con codigo 0 (EXIT_SUCCESS).\n"
    "  exit <codigo>  --> Finaliza con el codigo de salida especificado.\n"
    "\n"
    "Argumentos:\n"
    "  <codigo> : (opcional) Un numero entero que representa el codigo de salida.\n"
    "\n"
    "Notas:\n"
    "  - Si el argumento proporcionado no es un numero valido, se\n"
    "    mostrara un error y se devolvera el codigo 153.\n"
    "  - El programa establece una bandera interna de cerrado y\n"
    "    guarda el codigo de salida si es valido.\n",
    {0, 1},
    fn_exit,
};
