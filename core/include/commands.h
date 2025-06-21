#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "list.h"

typedef int (*command)(LIST_ptr);

typedef struct {
  char *name;
  char *short_desc;
  char *standar_desc;
  char *long_desc;
  int args_len[2];
  command func;
} Command;

extern const Command cmd_help;
extern const Command cmd_exit;
extern const Command cmd_new;
extern const Command cmd_del;
extern const Command cmd_set;
extern const Command cmd_get;
extern const Command cmd_echo;
extern const Command cmd_input;
extern const Command cmd_clear_history;
extern const Command cmd_resolve;
extern const Command cmd_arp_spoof;
extern const Command cmd_iface;
#endif
