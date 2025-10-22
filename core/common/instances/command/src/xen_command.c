#include "xen_command.h"
#include "callable.h"
#include "instance.h"
#include "program_code.h"
#include "xen_command_implement.h"
#include "xen_command_instance.h"
#include "xen_nil.h"

Xen_INSTANCE* Xen_Command_From_Native(Xen_Native_Func fn_cmd,
                                      Xen_INSTANCE* self,
                                      Xen_Instance* closure) {
  Xen_Command* cmd =
      (Xen_Command*)__instance_new(&Xen_Command_Implement, nil, 0);
  if (!cmd) {
    return nil;
  }
  cmd->cmd_callable = callable_new_native(fn_cmd);
  if (!cmd->cmd_callable) {
    Xen_DEL_REF(cmd);
    return nil;
  }
  if_nil_neval(self) cmd->self = Xen_ADD_REF(self);
  if_nil_neval(closure) cmd->closure = Xen_ADD_REF(closure);
  return (Xen_INSTANCE*)cmd;
}

Xen_INSTANCE* Xen_Command_From_Program(ProgramCode_t pc_cmd, Xen_INSTANCE* self,
                                       Xen_Instance* closure) {
  Xen_Command* cmd =
      (Xen_Command*)__instance_new(&Xen_Command_Implement, nil, 0);
  if (!cmd) {
    return nil;
  }
  cmd->cmd_callable = callable_new_code(pc_cmd);
  if (!cmd->cmd_callable) {
    Xen_DEL_REF(cmd);
    return nil;
  }
  if_nil_neval(self) cmd->self = Xen_ADD_REF(self);
  if_nil_neval(closure) cmd->closure = Xen_ADD_REF(closure);
  return (Xen_INSTANCE*)cmd;
}

Xen_Instance* Xen_Command_Call(Xen_Instance* cmd, Xen_Instance* args) {
  if (Xen_TYPE(cmd) != &Xen_Command_Implement) {
    return NULL;
  }
  Xen_Instance* ret = Xen_Command_Implement.__callable(0, cmd, args);
  if (!ret) {
    return NULL;
  }
  return ret;
}
