#include <locale.h>

#include "instance_life.h"
#include "list.h"
#include "logs.h"
#include "program.h"
#include "vm_def.h"
#include "xen_alloc.h"
#include "xen_life.h"
#include "xen_module_load.h"

int Xen_Init(int argc, char** argv) {
  program.argv = argv + 1;
  program.argc = argc - 1;

  global_logs = list_new();
  if (!global_logs) {
    return 0;
  }
  if (!vm_create()) {
    log_free(NULL);
    return 0;
  }
  setlocale(LC_CTYPE, "");

  if (!Xen_Instance_Init()) {
    vm_destroy();
    log_free(NULL);
    return 0;
  }

  if (!Xen_Module_Load_Startup()) {
    Xen_Instance_Finish();
    vm_destroy();
    log_free(NULL);
    return 0;
  }
  return 1;
}

void Xen_Finish() {
  Xen_Instance_Finish();
  Xen_Dealloc(program.name);
  vm_destroy();
  log_free(NULL);
}
