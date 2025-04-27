#include <stdlib.h>

#include "commands.h"
#include "list.h"
#include "program.h"

#define NAME "exit"

static int fn_exit(LIST_ptr args) {
  program.closed = 1;
  return EXIT_SUCCESS;
}

// Se crea la estructura del comando
const Command cmd_exit = {
    NAME,
    "Sale de la shell",
    fn_exit,
};
