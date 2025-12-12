#ifndef __READ_STRING_UTF8_H__
#define __READ_STRING_UTF8_H__

#include "list.h"
#include "string_utf8.h"

typedef struct {
  int COLS, ROWS;
} term_size;

typedef enum {
  KEY_NULL = 0,
  KEY_EOF = 1000,
  KEY_ESC,
  KEY_ARROW_UP,
  KEY_ARROW_DOWN,
  KEY_ARROW_RIGHT,
  KEY_ARROW_LEFT,
  KEY_TAB,
  KEY_ENTER,
  KEY_HOME,
  KEY_END,
  KEY_BACKSPACE,
  KEY_DELETE,
} key_value;

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
CodeUTF8 read_raw_char_utf8(void);
LIST_ptr read_string_utf8(void);

#endif
