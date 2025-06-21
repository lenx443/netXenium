#ifndef __TERMINAL_H__
#define __TERMINAL_H__

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

void terminal_raw_input_on();
void terminal_raw_input_off();
int read_raw_key();
term_size get_cursor_position();
int get_terminal_size(term_size *);

#endif
