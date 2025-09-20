#ifndef __SUGGESTION_H__
#define __SUGGESTION_H__

#include "list.h"
#include "macros.h"

typedef enum {
  COMMAND,
} suggest_types;

typedef struct {
  char sg_name[SUGGEST_NAME_LEN];
  char sg_value[SUGGEST_MAX_VALUE_SIZE];
  char sg_desc[SUGGEST_DESC_LEN];
  suggest_types sg_type;
} suggest_struct;

typedef struct {
  LIST_ptr suggestions;
  int suggest_showed;
} SUGGEST;

typedef SUGGEST *SUGGEST_ptr;

SUGGEST_ptr suggest_new();
int suggest_add(SUGGEST_ptr, const char *, const char *, const char *, suggest_types);
void suggest_show(SUGGEST_ptr, int, int);
void suggest_hide(SUGGEST_ptr, int);
suggest_struct *suggest_get(SUGGEST_ptr, int);
void suggest_clear(SUGGEST_ptr);
void suggest_free(SUGGEST_ptr);

#endif
