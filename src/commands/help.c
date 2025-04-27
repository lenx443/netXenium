#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "commands.h"
#include "list.h"
#include "program.h"

#define NAME "help"

/*
 * El commando `help` muestra la
 * ayuda de la shell, alistando
 * todos los comandos y sus
 * descripciones.
 */
static int fn_help(LIST_ptr args) {
  for (int i = 0; cmds[i] != NULL; i++) {
    printf(VERDE "%s" RESET " ——— %s\n", cmds[i]->name, cmds[i]->desc);
  };
  return EXIT_SUCCESS;
}

// Se crea la estructura del comando
const Command cmd_help = {
    NAME,
    "Muestra la ayuda",
    fn_help,
};
