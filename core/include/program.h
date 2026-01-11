#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include "history.h"

typedef struct {
  char** argv;
  int argc;
  char* name;
  int closed;
  int exit_code;
  int return_code;
} Program_State;

typedef enum {
  EXEC_MODE = 0,
  SUGGEST_MODE,
} ExecMode;

void shell_loop(void);

extern Program_State program;
extern HISTORY_ptr history;

#endif
