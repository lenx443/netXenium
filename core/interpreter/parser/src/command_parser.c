#include "command_parser.h"
#include "compiler.h"
#include "interpreter.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_except.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_string.h"
#include "xen_typedefs.h"

#include <ctype.h>

struct Args {
  char** values;
  int size;
  int cap;
};

struct CMDParser {
  const char* cmd;
  Xen_size_t curr_pos;
};

static struct Args* args_new(void);
static void args_push(struct Args*, char*);

static void cmd_advance(struct CMDParser*);
static const char* cmd_current(struct CMDParser*);
static void cmd_skip_whitespace(struct CMDParser*);
static int is_valid_token(char);

static int parse_tokens(struct Args*, struct CMDParser*);
static int eval_tokens(struct Args*, Xen_Instance*, Xen_Instance*, Xen_VM_Scopes*);

char** Xen_Command_Parser(const char *cmd, int *argc, Xen_Instance* globals, Xen_Instance* instances, Xen_VM_Scopes* scopes) {
  struct Args* args = args_new();
  struct CMDParser parser = {
    .cmd = cmd,
    .curr_pos = 0,
  };
  if (!parse_tokens(args, &parser)) {
    return NULL;
  }
  if (!eval_tokens(args, globals, instances, scopes)) {
    return NULL;
  }
  if (args->size > 0) {
    char* file = args->values[0];
    Xen_c_string_t format_string = "%s.nxm";
    Xen_ssize_t fsize = snprintf(NULL, 0, format_string, file);
    if (fsize == -1) {
      return NULL;
    }
    args->values[0] = Xen_Alloc(fsize + 1);
    snprintf(args->values[0], fsize + 1, format_string, file);
    Xen_Dealloc(file);
  }
  *argc = args->size;
  char **argv = args->values;
  Xen_Dealloc(args);
  return argv;
}

struct Args* args_new(void) {
  struct Args* args = Xen_Alloc(sizeof(struct Args));
  args->values = NULL;
  args->size = 0;
  args->cap = 0;
  return args;
}

void args_push(struct Args* args, char* val) {
  if (args->size >= args->cap) {
    args->cap = args->cap == 0 ? 4 : args->cap * 2;
    args->values = Xen_Realloc(args->values, sizeof(char*) * args->cap);
  }
  args->values[args->size++] = val;
}

void cmd_advance(struct CMDParser* parser) {
  parser->curr_pos++;
}

const char* cmd_current(struct CMDParser* parser) {
  return &parser->cmd[parser->curr_pos];
}

void cmd_skip_whitespace(struct CMDParser* parser) {
  while (1) {
    char c = parser->cmd[parser->curr_pos];
    if (c == ' ' || c == '\t') {
      cmd_advance(parser);
    } else {
      break;
    }
  }
}

int is_valid_token(char c) {
  return isalnum(c) || c == '_' || c == '.' || c == '=' || c == '$';
}

int parse_tokens(struct Args* args, struct CMDParser* parser) {
  while (1) {
    cmd_skip_whitespace(parser);
    const char* c = cmd_current(parser);
    if (*c == '\0') {
      break;
    } else if (*c == '\n') {
      cmd_advance(parser);
      break;
    } else if (*c == '$' && *(c + 1) == '(') {
      while (*cmd_current(parser) != ')' && *cmd_current(parser) != '\0') {
        cmd_advance(parser);
      }
      if (*cmd_current(parser) == '\0') {
        Xen_SyntaxError("Unclosed Expression.");
        return 0;
      }
      cmd_advance(parser);
      Xen_size_t len = cmd_current(parser) - c;
      args_push(args, Xen_CString_NDup(c, len));
    } else if (*c == '!' && *(c + 1) == '[') {
      while (*cmd_current(parser) != ']' && *cmd_current(parser) != '\0') {
        cmd_advance(parser);
      }
      if (*cmd_current(parser) == '\0') {
        Xen_SyntaxError("Unclosed Expression.");
        return 0;
      }
      cmd_advance(parser);
      Xen_size_t len = cmd_current(parser) - c;
      args_push(args, Xen_CString_NDup(c, len));
    } else if (*c == '"') {
      cmd_advance(parser);
      while (*cmd_current(parser) != '"' && *cmd_current(parser) != '\0') {
        cmd_advance(parser);
      }
      if (*cmd_current(parser) == '\0') {
        Xen_SyntaxError("Unclosed string.");
        return 0;
      }
      cmd_advance(parser);
      Xen_size_t len = cmd_current(parser) - c;
      args_push(args, Xen_CString_NDup(c, len));
    } else if (*c == '\'') {
      cmd_advance(parser);
      while (*cmd_current(parser) != '\'' && *cmd_current(parser) != '\0') {
        cmd_advance(parser);
      }
      if (*cmd_current(parser) == '\0') {
        Xen_SyntaxError("Unclosed string.");
        return 0;
      }
      cmd_advance(parser);
      Xen_size_t len = cmd_current(parser) - c;
      args_push(args, Xen_CString_NDup(c, len));
    } else if (*c == '(') {
      cmd_advance(parser);
      while (*cmd_current(parser) != ')' && *cmd_current(parser) != '\0') {
        cmd_advance(parser);
      }
      if (*cmd_current(parser) == '\0') {
        Xen_SyntaxError("Unclosed string.");
        return 0;
      }
      cmd_advance(parser);
      Xen_size_t len = cmd_current(parser) - c;
      args_push(args, Xen_CString_NDup(c, len));
    } else if (*c == '`') {
      cmd_advance(parser);
      const char* start = c + 1;
      while (*cmd_current(parser) != '`' && *cmd_current(parser) != '\0') {
        cmd_advance(parser);
      }
      if (*cmd_current(parser) == '\0') {
        Xen_SyntaxError("Unclosed string.");
        return 0;
      }
      Xen_size_t len = cmd_current(parser) - start;
      args_push(args, Xen_CString_NDup(start, len));
      cmd_advance(parser);
    } else if (is_valid_token(*c)) {
      while (is_valid_token(*cmd_current(parser))) {
        cmd_advance(parser);
      }
      Xen_size_t len = cmd_current(parser) - c;
      args_push(args, Xen_CString_NDup(c, len));
    } else {
      Xen_SyntaxError_Format("Unexpected token '%c'.", *c);
      return 0;
    }
  }
  return 1;
}

int eval_tokens(struct Args* args, Xen_Instance* globals, Xen_Instance* instances, Xen_VM_Scopes* scopes) {
  for (int i = 0; i < args->size; i++) {
    if (*args->values[i] == '$' && *(args->values[i] + 1) == '(') {
      const char* start = args->values[i] + 2;
      Xen_size_t len = Xen_CString_Len(start) - 1;
      char *expr = Xen_CString_NDup(start, len);
      Xen_Instance* result = interpreter("<cmd-expr>", expr, Xen_COMPILE_EXPR, globals, instances, scopes);
      if (!result) {
        Xen_Dealloc(expr);
        return 0;
      }
      Xen_Instance* str = Xen_Attr_String(result);
      char* r = Xen_CString_Dup(Xen_String_As_CString(str));
      char* prev_val = args->values[i];
      args->values[i] = r;
      Xen_Dealloc(prev_val);
      Xen_Dealloc(expr);
    }
  }
  if (args->values && *args->values[0] == '.') {
    if (strcmp(args->values[0], ".exit") == 0) {
      xen_globals->program->closed = 1;
    } else if (strcmp(args->values[0], ".local") == 0) {
      if (args->size != 4) {
        Xen_SyntaxError("Invalid declaration.");
        return 0;
      }
      if (strcmp(args->values[2], "=") == 0) {
        Xen_Instance* result = interpreter("<cmd-expr>", args->values[3], Xen_COMPILE_EXPR, globals, instances, scopes);
        if (!result) {
          return 0;
        }
        if (Xen_Map_Has_Str((Xen_Instance*)scopes->scopes->symbols->ptr, args->values[1])) {
          Xen_DeclError(args->values[1]);
          return 0;
        }
        Xen_Map_Push_Pair_Str((Xen_Instance*)scopes->scopes->symbols->ptr, (Xen_Map_Pair_Str){args->values[1], result});
      }
      else {
        Xen_SyntaxError("Invalid declaration.");
        return 0;
      }
    } else if (strcmp(args->values[0], ".var") == 0) {
      if (args->size != 4) {
        Xen_SyntaxError("Invalid declaration.");
        return 0;
      }
      if (strcmp(args->values[2], "=") == 0) {
        Xen_Instance* result = interpreter("<cmd-expr>", args->values[3], Xen_COMPILE_EXPR, globals, instances, scopes);
        if (!result) {
          return 0;
        }
        if (Xen_Map_Has_Str(instances, args->values[1])) {
          Xen_DeclError_Context(args->values[1]);
          return 0;
        }
        Xen_Map_Push_Pair_Str(instances, (Xen_Map_Pair_Str){args->values[1], result});
      }
      else {
        Xen_SyntaxError("Invalid declaration.");
        return 0;
      }
    } else if (strcmp(args->values[0], ".") == 0) {
      if (args->size != 2) {
        Xen_SyntaxError("Invalid statement.");
        return 0;
      }
      if (!interpreter("<cmd>", args->values[1], Xen_COMPILE_PROGRAM, globals, instances, scopes)) {
        return 0;
      }
    } else {
      Xen_SyntaxError_Format("Invalid keyword '%s'.", args->values[0]);
      return 0;
    }
    for (int i = 0; i < args->size; i++) {
      Xen_Dealloc(args->values[i]);
    }
    Xen_Dealloc(args->values);
    args->values = NULL;
    args->size = 0;
    args->cap = 0;
  } else if (args->values && *args->values[0] == '$') {
    if (args->size != 3) {
      Xen_SyntaxError("Invalid declaration.");
      return 0;
    }
    if (strcmp(args->values[1], "=") == 0) {
      Xen_Instance* result = interpreter("<cmd-expr>", args->values[2], Xen_COMPILE_EXPR, globals, instances, scopes);
      if (!result) {
        return 0;
      }
      Xen_Map_Push_Pair_Str((Xen_Instance*)Xen_VM()->globals_props->ptr, (Xen_Map_Pair_Str){args->values[0] + 1, result});
    }
    else {
      Xen_SyntaxError("Invalid declaration.");
      return 0;
    }
  }
  return 1;
}
