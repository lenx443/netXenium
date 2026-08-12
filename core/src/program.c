#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "async.h"
#include "attrs.h"
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
#include "xen_cstrings.h"
#include "xen_except.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_method.h"
#include "xen_module.h"
#include "xen_module_load.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

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
  return Xen_Program_Run_Command_Scopped(cmd, NULL, NULL, NULL);
}

int Xen_Program_Run_Command_Scopped(const char* cmd, Xen_Instance* globals, Xen_Instance* instances, Xen_VM_Scopes* scopes) {
  char **buffer = NULL;
  Xen_size_t size = 0;
  Xen_size_t cap = 0;

  int exit_code = 0;
  const char* v = NULL;
  for (const char *c = cmd;; c++) {
    if (*c == '$' && *(c + 1) == '(') {
      if (v == NULL) {
        const char* start = c + 2;
        while (*c != ')') {
          if (*c == '\0') {
            Xen_SyntaxError("Unclosed Expression");
            exit_code = 1;
            goto end;
          }
          c++;
        }
        Xen_size_t len = c - start;
        char *expr = Xen_Alloc(len + 1);
        strncpy(expr, start, len);
        expr[len] = '\0';
        printf("expr = %s\n", expr);
        Xen_Instance* result = interpreter("<cmd-expr>", expr, Xen_COMPILE_EXPR, globals, instances, scopes);
        Xen_Instance* str = Xen_Attr_String(result);
        char* r = Xen_CString_Dup(Xen_String_As_CString(str));
        if (cap <= size) {
          cap = (cap == 0) ? 4 : cap * 2;
          buffer = Xen_Realloc(buffer, cap * sizeof(char*));
        }
        buffer[size++] = r;
        Xen_Dealloc(expr);
      }
    } else if (*c == ' ' || *c == '\t' || *c == '\0') {
      if (v != NULL) {
        Xen_size_t len = c - v;
        char* r = Xen_Alloc(len + 1);
        strncpy(r, v, len);
        r[len] = '\0';
        if (cap <= size) {
          cap = (cap == 0) ? 4 : cap * 2;
          buffer = Xen_Realloc(buffer, cap * sizeof(char*));
        }
        buffer[size++] = r;
        v = NULL;
      }
      if (*c == '\0') {
        break;
      }
    } else {
      if (v == NULL) {
        v = c;
      }
    }
  }
  for (Xen_size_t i = 0; i < size; i++) {
    printf("buffer[%lu] = %s\n", i, buffer[i]);
  }
  if (size > 0) {
    char * file = buffer[0];
    Xen_c_string_t format_string = "%s.nxm";
    Xen_ssize_t fsize = snprintf(NULL, 0, format_string, file);
    if (fsize == -1) {
      return 1;
    }
    buffer[0] = Xen_Alloc(fsize + 1);
    snprintf(buffer[0], fsize + 1, format_string, file);
    Xen_Dealloc(file);
    exit_code = Xen_Program_Run_File(size, (const char**)buffer);
  }
end:
  for (Xen_size_t i = 0; i < size; i++) {
    Xen_Dealloc(buffer[i]);
  }
  Xen_Dealloc(buffer);
  return exit_code;
}

HISTORY_ptr history = NULL;
