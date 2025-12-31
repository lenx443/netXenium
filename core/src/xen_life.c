#include <locale.h>

#include "instance_life.h"
#include "program.h"
#include "source_file.h"
#include "vm_def.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_module_load.h"
#include "xen_nil.h"

void Xen_GetReady(void* globals) {
  xen_globals = globals;
}

int Xen_Init(int argc, char** argv) {
  xen_globals = Xen_Alloc(sizeof(struct Xen_Globals));
  program.argv = argv + 1;
  program.argc = argc - 1;
  xen_globals->program = &program;

  Xen_GC_GetReady();
  Xen_Instance_GetReady();
  xen_globals->true_instance = Xen_True_GetInstance();
  xen_globals->false_instance = Xen_False_GetInstance();
  xen_globals->nil_instance = Xen_Nil_GetInstance();
  Xen_IGC_Init();
  Xen_Source_Table_Init();
  if (!vm_create()) {
    Xen_Dealloc(xen_globals);
    return 0;
  }
  setlocale(LC_CTYPE, "");

  if (!Xen_Instance_Init()) {
    vm_destroy();
    Xen_Dealloc(xen_globals);
    return 0;
  }

  if (!Xen_Module_Load_Startup()) {
    Xen_Instance_Finish();
    vm_destroy();
    Xen_Dealloc(xen_globals);
    return 0;
  }
  Xen_GC_MinorCollect();
  return 1;
}

void Xen_Finish(void) {
  Xen_Instance_Finish();
  Xen_Dealloc(program.name);
  vm_destroy();
  Xen_Source_Table_Finish();
  Xen_IGC_Finish();
  Xen_GC_Shutdown();
  Xen_Dealloc(xen_globals);
}
struct Xen_Globals* xen_globals = NULL;
