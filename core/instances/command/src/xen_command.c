#include "callable.h"
#include "instance.h"
#include "program_code.h"
#include "xen_command.h"
#include "xen_command_implement.h"
#include "xen_command_instance.h"
#include "xen_nil.h"

Xen_INSTANCE *Xen_Command_From_Native(Xen_Native_Func fn_cmd, Xen_INSTANCE *self,
                                      Xen_Instance *closure) {
  Xen_Command *cmd = (Xen_Command *)__instance_new(&Xen_Command_Implement, nil, 0);
  if (!cmd) { return nil; }
  cmd->cmd_callable = callable_new_native(fn_cmd);
  if (!cmd->cmd_callable) {
    Xen_DEL_REF(cmd);
    return nil;
  }
  cmd->self = self;
  cmd->closure = closure;
  return (Xen_INSTANCE *)cmd;
}

Xen_INSTANCE *Xen_Command_From_Program(ProgramCode_t pc_cmd, Xen_INSTANCE *self,
                                       Xen_Instance *closure) {
  Xen_Command *cmd = (Xen_Command *)__instance_new(&Xen_Command_Implement, nil, 0);
  if (!cmd) { return nil; }
  cmd->cmd_callable = callable_new_code(pc_cmd);
  if (!cmd->cmd_callable) {
    Xen_DEL_REF(cmd);
    return nil;
  }
  cmd->self = self;
  cmd->closure = closure;
  return (Xen_INSTANCE *)cmd;
}
