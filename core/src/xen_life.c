#include <locale.h>

#include "instance_life.h"
#include "list.h"
#include "logs.h"
#include "program.h"
#include "source_file.h"
#include "vm_def.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_module_load.h"

int Xen_Init(int argc, char** argv) {
  program.argv = argv + 1;
  program.argc = argc - 1;

  Xen_IGC_Init();
  global_logs = list_new();
  if (!global_logs) {
    return 0;
  }
  Xen_Source_Table_Init();
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
  Xen_GC_Collect();
  return 1;
}

void Xen_Finish(void) {
  Xen_Instance_Finish();
  Xen_Dealloc(program.name);
  vm_destroy();
  Xen_Source_Table_Finish();
  log_free(NULL);
  Xen_IGC_Finish();
  Xen_GC_Shutdown();
}
