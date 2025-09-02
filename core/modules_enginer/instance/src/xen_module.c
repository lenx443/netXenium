#include "instance.h"
#include "xen_module.h"
#include "xen_module_implement.h"
#include "xen_module_instance.h"
#include "xen_nil.h"

Xen_Instance *Xen_Module_New(struct __Instances_Map *mod_map, Xen_Instance *mod_context) {
  Xen_Module *module = (Xen_Module *)__instance_new(&Xen_Module_Implement, nil, 0);
  if (!module) { return nil; }
  module->mod_map = mod_map;
  module->mod_context = mod_context;
  Xen_ADD_REF(mod_context);
  return (Xen_Instance *)module;
}
