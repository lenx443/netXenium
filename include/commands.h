#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "list.h"

typedef int (*command)(LIST_ptr);

typedef struct {
  char *name;
  char *desc;
  command func;
} Command;

extern const Command cmd_help;
extern const Command cmd_exit;
extern const Command cmd_set;
extern const Command cmd_get;
extern const Command cmd_resolve;
extern const Command cmd_arp_spoof;

#endif
