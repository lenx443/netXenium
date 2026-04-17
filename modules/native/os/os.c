#include "netxenium/netXenium.h"
#include "netxenium/xen_function.h"
#include <bits/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

static Xen_Instance* fn_system(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"cmd", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* args_binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!args_binding) {
    return NULL;
  }
  Xen_Instance* cmd = Xen_Function_ArgBinding_Search(args_binding, "cmd")->value;
  Xen_Function_ArgBinding_Free(args_binding);
  int sts = system(Xen_String_As_CString(cmd));
  return Xen_Number_From_Int(sts);
}

static Xen_Instance* fn_exec_out(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"cmd", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* args_binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!args_binding) {
    return NULL;
  }
  const char* cmd = Xen_String_As_CString(Xen_Function_ArgBinding_Search(args_binding, "cmd")->value);
  Xen_Function_ArgBinding_Free(args_binding);
  int pipefd[2];
  if (pipe(pipefd) < 0) return NULL;
  pid_t pid = fork();
  if (pid < 0) return NULL;
  if (pid == 0) {
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);
    const char* argv[] = {"/bin/sh", "-c", cmd, NULL};
    execvp(argv[0], (char* const*)argv);
    _exit(1);
  }
  close(pipefd[1]);
  Xen_CBuffer *buf = Xen_CBuffer_New();
  char tmp[1024];
  Xen_size_t n;
  while ((n = read(pipefd[0], tmp, sizeof(tmp) - 1)) > 0) {
    tmp[n] = '\0';
    Xen_CBuffer_Append_CStr(buf, tmp);
  }
  Xen_Instance* out = Xen_CBuffer_As_String(buf);
  Xen_CBuffer_Free(buf);
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status)) {
    Xen_Instance* result = Xen_Tuple_From_Array(2, (Xen_Instance *[]){
      out, Xen_Number_From_Int(WEXITSTATUS(status))
    });
    return result;
  }
  return NULL;
}

static Xen_Instance* fn_get_dir(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  char* dir = getcwd(NULL, 0);
  if (!dir) {
    return NULL;
  }
  Xen_Instance* result = Xen_String_From_CString(dir);
  free(dir);
  return result;
}

static Xen_Instance* fn_change_dir(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"d", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* args_binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!args_binding) {
    return NULL;
  }
  Xen_Instance* dir = Xen_Function_ArgBinding_Search(args_binding, "d")->value;
  Xen_Function_ArgBinding_Free(args_binding);
  int r = chdir(Xen_String_As_CString(dir));
  if (r != 0) {
    return NULL;
  }
  return nil;
}

static Xen_Instance* fn_list_dir(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  static Xen_Function_ArgSpec args_def[] = {
      {"d", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_STRING, XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* args_binding = Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!args_binding) {
    return NULL;
  }
  const char* path = ".";
  Xen_Function_ArgBound* dir_arg = Xen_Function_ArgBinding_Search(args_binding, "d");
  if (dir_arg->provided) {
    path = Xen_String_As_CString(dir_arg->value);
  }
  Xen_Function_ArgBinding_Free(args_binding);
  DIR* dir = opendir(path);
  if (!dir) {
    return NULL;
  }
  Xen_Instance* dirs = Xen_Vector_New();
  Xen_IGC_Push(dirs);
  struct dirent* entry = NULL;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    Xen_Vector_Push(dirs, Xen_String_From_CString(entry->d_name));
  }
  Xen_IGC_Pop();
  return dirs;
}

struct Xen_Module_Function functions[] = {
  {"system", fn_system},
  {"exec_out", fn_exec_out},
  {"get_dir", fn_get_dir},
  {"change_dir", fn_change_dir},
  {"list_dir", fn_list_dir},
  {NULL, NULL},
};

struct Xen_Module_Def* Xen_Module_os_Start(void*);
struct Xen_Module_Def* Xen_Module_os_Start(void* globals) {
  Xen_GetReady(globals);
  return Xen_Module_Define("os", NULL, functions, NULL);
}
