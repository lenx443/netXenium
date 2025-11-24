#include "xen_modules_def.h"
#include "m_core.h"
#include "m_interpreter.h"
#include "xen_module_types.h"

Xen_Module_Def_Table xen_startup_modules = {
    &Module_Core,
    &Module_Interpreter,
    NULL,
};
