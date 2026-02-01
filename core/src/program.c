#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
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
#include "xen_map.h"

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
    if (program.closed) {
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
      Xen_Dealloc(cmd_str);
      if (program.closed)
        break;
      continue;
    }
    Xen_Dealloc(cmd_str);
    if (program.closed)
      break;
  }
  Xen_IGC_XPOP(2);
  Xen_GC_Pop_Root();
  history_save(*history);
  history_free(history);
}

Program_State program = {
    NULL, 0, NULL, 0, EXIT_SUCCESS, EXIT_SUCCESS,
};
HISTORY_ptr history = NULL;
