#include <locale.h>
#include <stdlib.h>

#include "instance_life.h"
#include "list.h"
#include "logs.h"
#include "program.h"
#include "properties.h"
#include "vm_def.h"
#include "xen_life.h"
#include "xen_module_load.h"

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

  if (!Xen_Instance_Init()) {
    vm_destroy();
    log_free(NULL);
    prop_reg_free(prop_register);
    return 0;
  }

  if (!Xen_Module_Load_Startup()) {
    Xen_Instanse_Finish();
    vm_destroy();
    log_free(NULL);
    prop_reg_free(prop_register);
    return 0;
  }

  return 1;
}

void Xen_Finish() {
  Xen_Instanse_Finish();
  free(program.name);
  prop_reg_free(prop_register);
  vm_destroy();
  log_free(NULL);
}
