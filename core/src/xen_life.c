#include <locale.h>

#include "instance_life.h"
#include "source_file.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_nil.h"

void Xen_GetReady(void* globals) {
  xen_globals = globals;
}

int Xen_Init(void) {
  xen_globals = Xen_Alloc(sizeof(struct Xen_Globals));

  Xen_GC_GetReady();
  Xen_Instance_GetReady();
  xen_globals->true_instance = Xen_True_GetInstance();
  xen_globals->false_instance = Xen_False_GetInstance();
  xen_globals->nil_instance = Xen_Nil_GetInstance();
  Xen_IGC_Init();
  if (!Xen_Instance_Init()) {
    Xen_Dealloc(xen_globals);
    return 0;
  }
  Xen_Source_Table_Init();
  setlocale(LC_CTYPE, "");

  Xen_GC_MinorCollect();
  return 1;
}

void Xen_Finish(void) {
  Xen_Instance_Finish();
  Xen_Source_Table_Finish();
  Xen_IGC_Finish();
  Xen_GC_Shutdown();
  Xen_Dealloc(xen_globals);
}
struct Xen_Globals* xen_globals = NULL;
