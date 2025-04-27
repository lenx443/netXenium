#ifndef __PROGRAM_H__
#define __PROGRAM_H__

#include "commands.h"
typedef struct {
  int closed;
} Program_State;

void no_signal(int);

extern Program_State program;
extern const Command *cmds[];

#endif
