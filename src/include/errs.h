#ifndef __ERRS_H__
#define __ERRS_H__

#include "colors.h"

#define print_error(sender, text)                                              \
  fprintf(stderr,                                                              \
          "[" ROJO "!" RESET "] " AMARILLO "%s:%d %s" RESET                    \
          "\n    Error: %s\n",                                                 \
          sender, __LINE__, text, strerrs(code_error))

#define CHECK_ERROR(condition, text)                                           \
  if (condition) {                                                             \
    print_error(NAME, text);                                                   \
  }

#define CHECK_ERROR_RETURN(condition, text, ret)                               \
  if (condition) {                                                             \
    print_error(NAME, text);                                                   \
    return ret;                                                                \
  }

#define CHECK_ERROR_CODE(condition, text, code)                                \
  if (condition) {                                                             \
    print_error(NAME, text);                                                   \
    code;                                                                      \
  }

#define CHECK_ERROR_CODE_RETURN(condition, text, code, ret)                    \
  if (condition) {                                                             \
    print_error(NAME, text);                                                   \
    code;                                                                      \
    return ret;                                                                \
  }

char *strerrs(int);

extern int code_error;

#endif
