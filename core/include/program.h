#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include "history.h"
#include "vm_scope.h"
#include "vm_def.h"

typedef struct __Program {
  const char** argv;
  int argc;
  VM* vm;
  int mod_core_success;
  int closed;
  int exit_code;
  struct __Program* prev_program;
} Program_State;

void shell_loop(void);

void Xen_Program_Push(int, const char**);
int Xen_Program_Pop(void);

int Xen_Program_Run_File(int, const char **);
int Xen_Program_Run_Repl(void);
int Xen_Program_Run_Command(const char*);
int Xen_Program_Run_Command_Scopped(const char*, Xen_Instance*, Xen_Instance*, Xen_VM_Scopes*);
int Xen_Program_Run_Command_File(int, const char**);
int Xen_Program_Run_Command_Shell(void);

extern HISTORY_ptr history;

#endif
