#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "async.h"
#include "attrs.h"
#include "colors.h"
#include "command_parser.h"
#include "compiler.h"
#include "history.h"
#include "instance.h"
#include "interpreter.h"
#include "list.h"
#include "program.h"
#include "read_string_utf8.h"
#include "string_utf8.h"
#include "vm.h"
#include "vm_scope.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_method.h"
#include "xen_module.h"
#include "xen_module_load.h"
#include "xen_nil.h"
#include "xen_tuple.h"

void shell_loop(void) {
  printf(AZUL "NetXenium" RESET " (C) " AMARILLO "Lenx443 2024-2026" RESET "\n"
              "Type " VERDE "exit" RESET " for quit\n");
  const char* home = getenv("HOME");
  if (home == NULL) {
    printf("No se encontro la variable entorno HOME\n");
    return;
  };
  char history_path[1024];
  snprintf(history_path, 1024, "%s/.xenium_history", home);
  history = history_new(history_path);
  Xen_Instance* globals = Xen_Map_New();
  Xen_Instance* instances = Xen_Map_New();
  Xen_VM_Scopes* scopes = Xen_VM_Scopes_New();
  Xen_IGC_Push(globals);
  Xen_IGC_Push(instances);
  Xen_GC_Push_Root((Xen_GCHeader*)scopes);
  Xen_VM_Scopes_Push(scopes);
  while (1) {
#ifndef SHELL_BASIC
    LIST_ptr cmd = read_string_utf8();
    if (!cmd) {
      fputs("\n", stdout);
      if (Xen_VM_Except_Active()) {
        Xen_VM_Except_Backtrace_Show();
        continue;
      }
      break;
    }
    if (xen_globals->program->closed) {
      list_free(cmd);
      break;
    }
    char* cmd_str = string_utf8_get(cmd);
    list_free(cmd);
#else
    fputs(" -> ", stdout);
    char* cmd_str = Xen_Alloc(CMDSIZ);
    if (!fgets(cmd_str, CMDSIZ, stdin)) {
      fputs("\n", stdout);
      Xen_Dealloc(cmd_str);
      if (Xen_VM_Except_Active()) {
        Xen_VM_Except_Backtrace_Show();
        continue;
      }
      fputs("\n", stdout);
      break;
    }
#endif
    if (!interpreter("<stdin>", cmd_str, Xen_COMPILE_REPL, globals, instances, scopes)) {
      if (Xen_VM_Except_Active()) {
        Xen_VM_Except_Backtrace_Show();
      }
      Xen_Dealloc(cmd_str);
      if (xen_globals->program->closed)
        break;
      continue;
    }
    if (Xen_VM_Except_Active()) {
      Xen_VM_Except_Backtrace_Show();
      Xen_Dealloc(cmd_str);
      if (xen_globals->program->closed)
        break;
      continue;
    }
    Xen_Dealloc(cmd_str);
    if (xen_globals->program->closed)
      break;
  }
  Xen_IGC_XPOP(2);
  Xen_GC_Pop_Root();
  history_save(*history);
  history_free(history);
}

void Xen_Program_Push(int argc, const char **argv) {
  Program_State* program = Xen_ZAlloc(1, sizeof(Program_State));
  program->argc = argc;
  program->argv = argv;
  program->prev_program = xen_globals->program;
  xen_globals->program = program;
  if (!vm_create()) {
    abort();
  }
  if (!Xen_Module_Load_Startup()) {
    abort();
  }
}

int Xen_Program_Pop(void) {
  vm_destroy();
  int exit_code = xen_globals->program->exit_code;
  Program_State* program = xen_globals->program;
  xen_globals->program = program->prev_program;
  Xen_Dealloc(program);
  return exit_code;
}

int Xen_Program_Run_File(int argc, const char **argv) {
    Xen_Program_Push(argc, argv);
    const char* slash = strrchr(argv[0], '/');
    Xen_Instance* module = NULL;
    if (slash) {
      size_t len = slash - argv[0];
      char* dir = Xen_Alloc(len + 1);
      memcpy(dir, argv[0], len);
      dir[len] = '\0';
      module = Xen_Module_Load(argv[0], "<start>", dir, XEN_MODULE_GUEST);
      Xen_Dealloc(dir);
    } else {
      module = Xen_Module_Load(argv[0], "<start>", ".", XEN_MODULE_GUEST);
    }
    if (Xen_VM_Except_Active()) {
      Xen_VM_Except_Backtrace_Show();
    }
    if (module) {
      Xen_Instance* start = Xen_Attr_Get_Str(module, "$__start");
      if (!start) {
        return Xen_Program_Pop();
      }
      if (Xen_IMPL(start) != xen_globals->implements->method) {
        return Xen_Program_Pop();
      }
      Xen_Instance* args = nil;
      if (Xen_SIZE(Xen_Method_Args(start)) > 0) {
        args = Xen_Tuple_From_Array(1, (Xen_Instance**)&Xen_VM()->args->ptr);
      }
      if (Xen_Method_IsAsync(start)) {
        Xen_Instance* coro = Xen_Method_Call(start, args, nil);
        Xen_Async_Run(coro);
      } else {
        Xen_Method_Call(start, args, nil);
      }
      if (Xen_VM_Except_Active()) {
        Xen_VM_Except_Backtrace_Show();
      }
    }
    return Xen_Program_Pop();
}

int Xen_Program_Run_Repl(void) {
    Xen_Program_Push(0, NULL);
    shell_loop();
    return Xen_Program_Pop();
}

int Xen_Program_Run_Command(const char* cmd) {
  return Xen_Program_Run_Command_Scopped(cmd, NULL, NULL, NULL, NULL);
}

int Xen_Program_Run_Command_Scopped(const char* cmd, Xen_Instance* alias, Xen_Instance* globals, Xen_Instance* instances, Xen_VM_Scopes* scopes) {
  int exit_code = 0;
  int argc;
  int type = 0;
  char **argv = Xen_Command_Parser(cmd, &argc, &type, alias, globals, instances, scopes);
  if (!argv) {
    return 1;
  }
  if (type == 1) {
    exit_code = Xen_Program_Run_File(argc, (const char**)argv);
  } else if (type == 2) {
    exit_code = Xen_Program_Run_Command_File(argc, (const char**)argv);
  }
  for (int i = 0; i < argc; i++) {
    Xen_Dealloc(argv[i]);
  }
  Xen_Dealloc(argv);
  return exit_code;
}

int Xen_Program_Run_Command_File(int argc, const char** argv) {
  Xen_Program_Push(argc, argv);
  Xen_Instance* alias = Xen_Map_New();
  Xen_Instance* globals = Xen_Map_New();
  Xen_Instance* instances = Xen_Map_New();
  Xen_VM_Scopes* scopes = Xen_VM_Scopes_New();
  Xen_IGC_Push(alias);
  Xen_IGC_Push(globals);
  Xen_IGC_Push(instances);
  Xen_GC_Push_Root((Xen_GCHeader*)scopes);
  Xen_VM_Scopes_Push(scopes);
  FILE* fp = fopen(argv[0], "r");
  if (!fp) {
    return 1;
  }
  char line[CMDSIZ];
  while (fgets(line, CMDSIZ, fp)) {
    Xen_Program_Run_Command_Scopped(line, alias, globals, instances, scopes);
    if (Xen_VM_Except_Active()) {
      Xen_VM_Except_Backtrace_Show();
      if (xen_globals->program->closed)
        break;
      continue;
    }
    if (xen_globals->program->closed)
      break;
  }
  fclose(fp);
  Xen_IGC_XPOP(3);
  Xen_GC_Pop_Root();
  return Xen_Program_Pop();
}

int Xen_Program_Run_Command_Shell(void) {
  Xen_Program_Push(0, NULL);
  printf(AZUL "NetXenium [SHELL]" RESET " (C) " AMARILLO "Lenx443 2024-2026" RESET "\n"
              "Type " VERDE ".exit" RESET " for quit\n");
  const char* home = getenv("HOME");
  if (home == NULL) {
    printf("No se encontro la variable entorno HOME\n");
    return 1;
  };
  char history_path[1024];
  snprintf(history_path, 1024, "%s/.xenium_sh_history", home);
  history = history_new(history_path);
  Xen_Instance* alias = Xen_Map_New();
  Xen_Instance* globals = Xen_Map_New();
  Xen_Instance* instances = Xen_Map_New();
  Xen_VM_Scopes* scopes = Xen_VM_Scopes_New();
  Xen_IGC_Push(alias);
  Xen_IGC_Push(globals);
  Xen_IGC_Push(instances);
  Xen_GC_Push_Root((Xen_GCHeader*)scopes);
  Xen_VM_Scopes_Push(scopes);
  while (1) {
#ifndef SHELL_BASIC
    LIST_ptr cmd = read_string_utf8();
    if (!cmd) {
      fputs("\n", stdout);
      if (Xen_VM_Except_Active()) {
        Xen_VM_Except_Backtrace_Show();
        continue;
      }
      break;
    }
    if (xen_globals->program->closed) {
      list_free(cmd);
      break;
    }
    char* cmd_str = string_utf8_get(cmd);
    list_free(cmd);
#else
    fputs(" -> ", stdout);
    char* cmd_str = Xen_Alloc(CMDSIZ);
    if (!fgets(cmd_str, CMDSIZ, stdin)) {
      fputs("\n", stdout);
      Xen_Dealloc(cmd_str);
      if (Xen_VM_Except_Active()) {
        Xen_VM_Except_Backtrace_Show();
        continue;
      }
      fputs("\n", stdout);
      break;
    }
#endif
    Xen_Program_Run_Command_Scopped(cmd_str, alias, globals, instances, scopes);
    if (Xen_VM_Except_Active()) {
      Xen_VM_Except_Backtrace_Show();
      Xen_Dealloc(cmd_str);
      if (xen_globals->program->closed)
        break;
      continue;
    }
    Xen_Dealloc(cmd_str);
    if (xen_globals->program->closed)
      break;
  }
  Xen_IGC_XPOP(3);
  Xen_GC_Pop_Root();
  history_save(*history);
  history_free(history);
  return Xen_Program_Pop();
}

HISTORY_ptr history = NULL;
