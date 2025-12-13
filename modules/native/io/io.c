#include "netXenium.h"

struct Xen_Module_Def* Xen_Module_io_Start(void);
struct Xen_Module_Def* Xen_Module_io_Start(void) {
  return Xen_Module_Define("io");
}
