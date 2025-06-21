#include <asm-generic/termbits-common.h>
#include <asm-generic/termbits.h>
#include <asm-generic/termios.h>
#include <stddef.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "logs.h"
#include "terminal.h"

static struct termios original_terminal_mode;

void terminal_raw_input_on() {
  tcgetattr(STDIN_FILENO, &original_terminal_mode);

  struct termios raw_mode = original_terminal_mode;
  raw_mode.c_lflag &= ~(ECHO | ICANON);
  raw_mode.c_iflag &= ~(IXON | ICRNL);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_mode);
}

void terminal_raw_input_off() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_mode); }

int read_raw_key() {
  char c;
  size_t n;
  while ((n = read(STDIN_FILENO, &c, 1)) != 1)
    if (n == -1) return KEY_NULL;

  if ((unsigned char)c == 4) return KEY_EOF;
  if ((unsigned char)c == 127) return KEY_BACKSPACE;

  if (c == '\x1b') {
    char seq[3] = {0};
    size_t i;

    for (i = 0; i < 2; i++) {
      if (read(STDIN_FILENO, &seq[i], 1) != 1) return KEY_NULL;
    }

    if (seq[0] == '[') {
      switch (seq[1]) {
      case '3': {
        char tilde;
        if (read(STDIN_FILENO, &tilde, 1) != 1) return KEY_NULL;
        if (tilde == '~') return KEY_DELETE;
        break;
      }
      case 'A': return KEY_ARROW_UP;
      case 'B': return KEY_ARROW_DOWN;
      case 'C': return KEY_ARROW_RIGHT;
      case 'D': return KEY_ARROW_LEFT;
      case 'H': return KEY_HOME;
      case 'F': return KEY_END;
      }
    }
    return KEY_NULL;
  }
  return (int)c;
}

term_size get_cursor_position() {
  term_size out = {0};
  char ch;
  char buf[32];
  int i = 0;

  write(STDOUT_FILENO, "\033[6n", 4);
  fsync(STDOUT_FILENO);

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &ch, 1) != 1) break;
    buf[i++] = ch;
    if (ch == 'R') break;
  }
  buf[i] = '\0';

  if (buf[0] == '\033' && buf[1] == '[') { sscanf(buf + 2, "%d;%d", &out.ROWS, &out.COLS); }
  return out;
}

int get_terminal_size(term_size *tz) {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) < 0) {
    log_add(NULL, ERROR, "GetTerminalSize", "{ioctl}(fd, op, ...)");
    log_add(NULL, ERROR, "GetTerminalSize", "Error al obtener el tamaño de la terminal");
    log_add_errno(NULL, ERROR, "GetTerminalSize");
    return 0;
  }
  tz->COLS = w.ws_col;
  tz->ROWS = w.ws_row;
  return 1;
}
