#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include "commands.h"
#include "history.h"
#include "suggestion.h"

typedef struct {
  char **argv;
  int argc;
  char *name;
  int closed;
  int exit_code;
  int return_code;
} Program_State;

typedef enum {
  EXEC_MODE = 0,
  SUGGEST_MODE,
} ExecMode;

int command_parser(char *, ExecMode, SUGGEST_ptr *, int);
void load_script(char *);
void shell_loop(char *);

extern Program_State program;
extern const Command *cmds_table[];
extern HISTORY_ptr history;

#endif
