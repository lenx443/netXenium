#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "history.h"
#include "program.h"

#define NAME "clear_history"

static int fn_clear_history(LIST_ptr args) {
  if (history == NULL) return EXIT_FAILURE;
  FILE *history_file = fopen(history->filename, "w");
  if (!history_file) return EXIT_FAILURE;
  fclose(history_file);
  const char *filename = history->filename;
  history_free(history);
  history = NULL;
  history = history_new(filename);
  if (!history) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

const Command cmd_clear_history = {
    NAME,
    "limpia historial",
    "Borra el contenido del archivo de historial",
    "Comando: clear_history\n"
    "Descripcion:\n"
    "  Borra el historial de comandos almacenado en memoria y en archivo.\n"
    "\n"
    "Uso:\n"
    "  clear_history\n"
    "\n"
    "Funcionamiento:\n"
    "  - Si no existe historial, retorna error.\n"
    "  - Abre el archivo de historial en modo escritura para vaciarlo.\n"
    "  - Libera la estructura de historial en memoria.\n"
    "  - Crea una nueva estructura de historial usando el mismo archivo.\n"
    "  - Retorna éxito si el historial se ha limpiado correctamente.\n"
    "\n"
    "Errores comunes:\n"
    "  - Error si no existe historial.\n"
    "  - Error si no se puede abrir el archivo para limpiar.\n"
    "  - Error si no se puede recrear la estructura de historial.\n",
    {0, 0},
    fn_clear_history,
};
