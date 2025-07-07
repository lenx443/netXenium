#ifndef __RAW_ARGUMENTS_H__
#define __RAW_ARGUMENTS_H__

#include "list.h"

typedef enum {
  RAT_ARGUMENT = 0,
  RAT_PROPERTY,
  RAT_CONCAT,
} Raw_Argument_Types;

typedef struct {
  Raw_Argument_Types ra_type;
  union {
    int no_content;
    char *argument;
    char *prop;
  } ra_content;
} Raw_Argument;

typedef LIST_ptr Raw_Arguments;

int raw_args_push_argument(Raw_Arguments, char *);
int raw_args_push_property(Raw_Arguments, char *);
int raw_args_push_concat(Raw_Arguments);
void raw_args_free(Raw_Arguments);

#endif
