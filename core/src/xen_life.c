#include <locale.h>

#include "instance_life.h"
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
  Xen_Source_Table_Init();
  if (!vm_create()) {
    return 0;
  }
  setlocale(LC_CTYPE, "");

  if (!Xen_Instance_Init()) {
    vm_destroy();
    return 0;
  }

  if (!Xen_Module_Load_Startup()) {
    Xen_Instance_Finish();
    vm_destroy();
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
  Xen_IGC_Finish();
  Xen_GC_Shutdown();
}
