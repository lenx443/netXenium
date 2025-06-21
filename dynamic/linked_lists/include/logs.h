#ifndef __LOGS_H__
#define __LOGS_H__

#include <stddef.h>
#include <time.h>

#include "list.h"
#include "macros.h"

typedef enum {
  ERROR = 0,
  WARNING,
  INFO,
} log_types;

typedef struct {
  char source[LOG_SOURCE_LEN];
  char content[LOG_CONTENT_LEN];
  log_types level;
  time_t tm;
} log_struct;

int is_loged(LIST_ptr, char *);
void log_add(LIST_ptr, log_types, char *, char *, ...);
void log_add_errno(LIST_ptr, log_types, char *);
void log_get(LIST_ptr, char *, size_t);
void log_show(LIST_ptr);
void log_show_and_clear(LIST_ptr);
void log_file_save(LIST_ptr, char *);
void log_clear(LIST_ptr);
void log_free(LIST_ptr);

extern LIST_ptr global_logs;

#endif
