#include <signal.h>

#include "commands.h"
#include "program.h"

void no_signal(int sig) { signal(sig, no_signal); }
Program_State program = {0};

const Command *cmds[] = {
    &cmd_help,    &cmd_exit,      &cmd_get, &cmd_set,
    &cmd_resolve, &cmd_arp_spoof, NULL,
};
