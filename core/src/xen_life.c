#include <locale.h>
#include <stdlib.h>

#include "command_implement.h"
#include "list.h"
#include "logs.h"
#include "program.h"
#include "properties.h"
#include "vm_def.h"
#include "xen_life.h"

int Xen_Init(int argc, char **argv) {
  global_logs = list_new();
  if (!global_logs) { return 0; }
  if (!vm_create()) {
    log_free(NULL);
    return 0;
  }
  prop_register = prop_reg_new();
  if (prop_register == NULL) {
    log_add(NULL, ERROR, argv[0], "No se pudo iniciar la configuracion");
    log_show_and_clear(NULL);
    vm_destroy();
    log_free(NULL);
    return 0;
  }
  setlocale(LC_CTYPE, "");

  program.argv = argv;
  program.argc = argc;

  return 1;
}

void Xen_Finish() {
  free(program.name);
  prop_reg_free(prop_register);
  vm_destroy();
  log_free(NULL);
}
