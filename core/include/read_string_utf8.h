#ifndef __READ_STRING_UTF8_H__
#define __READ_STRING_UTF8_H__

#include "list.h"
#include "string_utf8.h"
#include "terminal.h"

typedef enum { SPECIAL_KEY = 0, CHAR_UTF8 } CodeTypes;

typedef struct {
  CodeTypes type;
  union {
    key_value code;
    CharUTF8 character;
  } value;
} CodeUTF8;

CodeUTF8 make_code_utf8_code(key_value);
CodeUTF8 make_code_utf8_char(CharUTF8);
CodeUTF8 read_raw_char_utf8();
LIST_ptr read_string_utf8();

#endif
