#include "m_async.h"
#include "m_config.h"
#include "m_core.h"
#include "m_interpreter.h"
#include "m_program.h"
#include "xen_module_types.h"
#include "xen_modules_def.h"

Xen_Module_Def_Table xen_startup_modules = {
    &Module_Core,  &Module_Interpreter, &Module_Config,
    &Module_Async, &Module_Program,     NULL,
};
