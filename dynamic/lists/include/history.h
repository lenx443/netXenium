#ifndef __HISTORY_H__
#define __HISTORY_H__

#include <stdint.h>

#include "list.h"
#include "macros.h"

typedef struct {
  char command[CMDSIZ];
} HISTORY_struct;

typedef struct {
  char filename[1024];
  LIST_ptr cache_history;
  LIST_ptr local_history;
} HISTORY;

typedef HISTORY *HISTORY_ptr;

HISTORY_ptr history_new(const char *);
int history_push_line(HISTORY_ptr, HISTORY_struct);
HISTORY_struct *history_get(HISTORY, int);
int history_save(HISTORY);
int history_size(HISTORY);
void history_free(HISTORY_ptr);

#endif
