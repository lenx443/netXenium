#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "list.h"
#include "logs.h"
#include "program.h"
#include "read_string_utf8.h"
#include "string_utf8.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_except.h"

static term_size current_term_size = {0};
static struct termios original_terminal_mode;

static void terminal_raw_input_on() {
  tcgetattr(STDIN_FILENO, &original_terminal_mode);

  struct termios raw_mode = original_terminal_mode;
  raw_mode.c_lflag &= ~(ECHO | ICANON);
  raw_mode.c_iflag &= ~(IXON | ICRNL);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_mode);
}

static int strip_ansi_escape_strlen(const char* str) {
  int len = 0;
  int in_escape = 0;

  for (int i = 0; str[i]; i++) {
    if (str[i] == '\033') {
      in_escape = 1;
      continue;
    }
    if (in_escape) {
      if (str[i] == 'm') {
        in_escape = 0;
      }
      continue;
    }
    len++;
  }
  return len;
}

static void terminal_raw_input_off() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_mode);
}

static term_size get_cursor_position() {
  term_size out = {0};
  char ch;
  char buf[32];
  size_t i = 0;

  write(STDOUT_FILENO, "\033[6n", 4);
  fsync(STDOUT_FILENO);

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &ch, 1) != 1)
      break;
    buf[i++] = ch;
    if (ch == 'R')
      break;
  }
  buf[i] = '\0';

  if (buf[0] == '\033' && buf[1] == '[') {
    sscanf(buf + 2, "%d;%d", &out.ROWS, &out.COLS);
  }
  return out;
}

static int get_terminal_size(term_size* tz) {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) < 0) {
    log_add(NULL, ERROR, "GetTerminalSize", "{ioctl}(fd, op, ...)");
    log_add(NULL, ERROR, "GetTerminalSize",
            "Error al obtener el tamaño de la terminal");
    log_add_errno(NULL, ERROR, "GetTerminalSize");
    return 0;
  }
  tz->COLS = w.ws_col;
  tz->ROWS = w.ws_row;
  return 1;
}

static void handle_winch(int sig) {
  (void)(sig);
  if (!get_terminal_size(&current_term_size)) {
    log_add(NULL, ERROR, "SHELL",
            "Ocurrio un problema al obtener el tamaño de la terminal");
    log_show_and_clear(NULL);
  }
}

static int get_scroll(int cursor_index, int scroll_offset, const char* prompt) {
  int prompt_len = strip_ansi_escape_strlen(prompt);
  int cols = current_term_size.COLS - prompt_len;

  if (cursor_index < scroll_offset) {
    scroll_offset = cursor_index;
  } else if (cursor_index >= scroll_offset + cols) {
    scroll_offset = cursor_index - cols + 1;
  }

  return scroll_offset;
}

static void show_cmd(term_size prompt_end, LIST_ptr cmd, int cursor_index,
                     int* scroll_offset) {
  int visible_cols = current_term_size.COLS - prompt_end.COLS - 2;
  int cmd_len = list_size(*cmd);

  if (cursor_index < *scroll_offset) {
    *scroll_offset = cursor_index;
  } else if (cursor_index >= *scroll_offset + visible_cols) {
    *scroll_offset = cursor_index - visible_cols + 1;
  }

  int printed_cols = 0;
  int char_start = 0;
  int col_accum = 0;

  for (; char_start < cmd_len; char_start++) {
    CharUTF8 ch = string_utf8_index_get(cmd, char_start);
    int w = char_utf8_display_with(ch);
    if (col_accum + w > *scroll_offset)
      break;
    col_accum += w;
  }

  printf("\033[%dG\033[K", prompt_end.COLS);

  for (int i = char_start; i < cmd_len && printed_cols < visible_cols; i++) {
    CharUTF8 ch = string_utf8_index_get(cmd, i);
    int w = char_utf8_display_with(ch);
    if (printed_cols + w > visible_cols)
      break;
    fwrite(ch.ch, 1, ch.size, stdout);
    printed_cols += w;
  }

  int rel_cursor_col = cursor_index - *scroll_offset;
  printf("\033[%dG", prompt_end.COLS + rel_cursor_col);

  fflush(stdout);
}

static int read_stdin_timed(char* buf, size_t bufsize, int time_ms) {
  fd_set readfds;
  struct timeval timeout;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);

  timeout.tv_sec = time_ms / 1000;
  timeout.tv_usec = (time_ms % 1000) * 1000;

  int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
  if (ready == -1)
    return -1;
  else if (ready == 0)
    return 0;
  else
    return read(STDIN_FILENO, buf, bufsize);
}

CodeUTF8 make_code_utf8_code(key_value code) {
  CodeUTF8 code_utf8 = {
      .type = SPECIAL_KEY,
      .value.code = code,
  };
  return code_utf8;
}

CodeUTF8 make_code_utf8_char(CharUTF8 ch) {
  CodeUTF8 code_utf8 = {
      .type = CHAR_UTF8,
      .value.character = ch,
  };
  return code_utf8;
}

CodeUTF8 read_raw_char_utf8() {
  char c;
  size_t n;
  while ((n = read(STDIN_FILENO, &c, 1)) != 1)
    if (n == (size_t)-1)
      return make_code_utf8_code(KEY_NULL);

  if ((unsigned char)c == 0x03) {
    Xen_Interrupt();
    return make_code_utf8_code(KEY_NULL);
  }
  if ((unsigned char)c == 4)
    return make_code_utf8_code(KEY_EOF);
  if ((unsigned char)c == 127)
    return make_code_utf8_code(KEY_BACKSPACE);
  if ((unsigned char)c == '\n' || (unsigned char)c == '\r')
    return make_code_utf8_code(KEY_ENTER);
  if ((unsigned char)c == '\t')
    return make_code_utf8_code(KEY_TAB);

  if (c == '\x1b') {
    char seq[3] = {0};
    size_t i;

    for (i = 0; i < 2; i++) {
      int result = read_stdin_timed(&seq[i], 1, 30);
      if (result == -1)
        return make_code_utf8_code(KEY_NULL);
      else if (result == 0)
        return make_code_utf8_code(KEY_ESC);
      else if (result != 1)
        return make_code_utf8_code(KEY_NULL);
    }

    if (seq[0] == '[') {
      switch (seq[1]) {
      case '3': {
        char tilde;
        if (read(STDIN_FILENO, &tilde, 1) != 1)
          return make_code_utf8_code(KEY_NULL);
        if (tilde == '~')
          return make_code_utf8_code(KEY_DELETE);
        break;
      }
      case 'A':
        return make_code_utf8_code(KEY_ARROW_UP);
      case 'B':
        return make_code_utf8_code(KEY_ARROW_DOWN);
      case 'C':
        return make_code_utf8_code(KEY_ARROW_RIGHT);
      case 'D':
        return make_code_utf8_code(KEY_ARROW_LEFT);
      case 'H':
        return make_code_utf8_code(KEY_HOME);
      case 'F':
        return make_code_utf8_code(KEY_END);
      }
    }
    return make_code_utf8_code(KEY_NULL);
  }
  int char_size = 0;
  if ((c & 0x80) == 0x00)
    char_size = 1;
  else if ((c & 0xE0) == 0xC0)
    char_size = 2;
  else if ((c & 0xF0) == 0xE0)
    char_size = 3;
  else if ((c & 0xF8) == 0xF0)
    char_size = 4;
  else
    return make_code_utf8_code(KEY_NULL);

  CharUTF8 character = {.size = (char_size > 0) ? char_size : 1};
  character.ch[0] = (unsigned char)c;

  for (int i = 1; i < char_size; i++) {
    int result = read_stdin_timed(&character.ch[i], 1, 30);
    if (result != 1)
      return make_code_utf8_code(KEY_NULL);
  }
  if (char_size == 3 && character.ch[0] == '\xEF' &&
      character.ch[1] == '\xB8' && character.ch[2] == '\x8F')
    return make_code_utf8_code(KEY_NULL);
  return make_code_utf8_char(character);
}

LIST_ptr read_string_utf8() {
#define default_promp(prompt) sprintf(prompt, " -> ");
  LIST_ptr cmd = list_new();
  int i = 0;
  CodeUTF8 c;

  char prompt[100] = "\0";
  default_promp(prompt);
  printf("%s", prompt);
  fflush(stdout);

  int history_position = -1;
  int cursor_index = 0;
  int cursor_of_i = 0;
  int scroll_offset = 0;

  terminal_raw_input_on();

  term_size prompt_end = get_cursor_position();

  handle_winch(SIGWINCH);
  signal(SIGWINCH, handle_winch);

  while (1) {
    c = read_raw_char_utf8();
    if (c.type == SPECIAL_KEY) {
      if (c.value.code == KEY_NULL) {
        goto error;
      }

      if (c.value.code == KEY_EOF) {
        if (i != 0)
          continue;
        program.closed = 1;
        printf("\n");
        break;
      }
      if (c.value.code == KEY_ARROW_UP) {
        if ((history_position + 1) < history_size(*history)) {
          history_position++;
          HISTORY_struct* history_value =
              history_get(*history, history_position);
          if (history_value == NULL)
            continue;
          list_clear(cmd);
          string_utf8_push_back(cmd, history_value->command);
          cursor_index = string_utf8_display_width(cmd);
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
          i = list_size(*cmd);
          cursor_of_i = i;
          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
        }
        continue;
      }
      if (c.value.code == KEY_ARROW_DOWN) {
        history_position--;
        if (history_position < 0) {
          list_clear(cmd);
          cursor_index = 0;
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
          i = 0;
          cursor_of_i = 0;

          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
          history_position = -1;
          continue;
        }
        HISTORY_struct* history_value = history_get(*history, history_position);
        if (history_value == NULL)
          continue;
        list_clear(cmd);
        string_utf8_push_back(cmd, history_value->command);
        cursor_index = string_utf8_display_width(cmd);
        scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
        i = list_size(*cmd);
        cursor_of_i = i;
        show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
        continue;
      }
      if (c.value.code == KEY_ARROW_LEFT) {
        if (cursor_index > 0) {
          cursor_of_i--;
          NODE_ptr node_ch;
          if ((node_ch = list_index_get(cursor_of_i, *cmd)) == NULL)
            continue;
          CharUTF8* ch = (CharUTF8*)node_ch->point;
          cursor_index -= char_utf8_display_with(*ch);
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
          continue;
        }
        continue;
      }
      if (c.value.code == KEY_ARROW_RIGHT) {
        if (cursor_of_i < i) {
          NODE_ptr node_ch = list_index_get(cursor_of_i, *cmd);
          if (node_ch == NULL)
            continue;
          CharUTF8* ch = (CharUTF8*)node_ch->point;
          cursor_index += char_utf8_display_with(*ch);
          cursor_of_i++;
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
        }
        continue;
      }
      if (c.value.code == KEY_HOME) {
        cursor_index = 0;
        cursor_of_i = 0;
        scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
        show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
        continue;
      }
      if (c.value.code == KEY_END) {
        cursor_index = string_utf8_display_width(cmd);
        cursor_of_i = list_size(*cmd);
        scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
        show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
        continue;
      }
      if (c.value.code == KEY_ENTER) {
        printf("\n");
        if (cmd->head != NULL) {
          HISTORY_struct* previus_history = history_get(*history, 0);
          if (previus_history == NULL ||
              string_utf8_strcmp_cstring(cmd, previus_history->command) != 0) {
            HISTORY_struct new_history_line;
            char* cmd_cstring = string_utf8_get(cmd);
            if (cmd_cstring == NULL) {
              log_add(NULL, ERROR, "SHELL",
                      "No se pudo gurdar el comando en el hsitorial");
              log_show_and_clear(NULL);
            } else {
              int cmd_size = Xen_CString_Len(cmd_cstring);
              strncpy(new_history_line.command, cmd_cstring, cmd_size);
              new_history_line.command[cmd_size] = '\0';
              history_push_line(history, new_history_line);
              Xen_Dealloc(cmd_cstring);
            }
          }
        }
        history_position = -1;
        break;
      }
      if (c.value.code == KEY_TAB) {
        continue;
      }
      if (c.value.code == KEY_BACKSPACE) {
        if (cursor_of_i > 0) {
          i--;
          cursor_of_i--;
          NODE_ptr node_ch = NULL;
          if ((node_ch = list_index_get(cursor_of_i, *cmd)) == NULL) {
            log_add(NULL, ERROR, "SHELL",
                    "no se pudo acceder al indice %d en cmd", cursor_of_i);
            continue;
          }
          cursor_index -= char_utf8_display_with(*(CharUTF8*)node_ch->point);
          list_erase_at_index(cmd, cursor_of_i);
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);

          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
          continue;
        }
        continue;
      } else if (c.value.code == KEY_DELETE) {
        if (cursor_index < i) {
          list_erase_at_index(cmd, cursor_of_i - 1);
          i--;
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);

          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
          continue;
        }
        continue;
      }
    } else if (c.value.character.ch[0] != '\0' && c.value.character.size != 0) {
      if (!list_push_at_index(cmd, cursor_of_i, &c.value.character,
                              sizeof(CharUTF8))) {
        DynSetLog(NULL);
        log_add(NULL, ERROR, "SHELL",
                "No se pudo agregar el caracter en en indice %d", cursor_of_i);
        log_add(NULL, ERROR, "SHELL", "Caracter que cuaso el error: %s",
                c.value.character.ch);
        log_show_and_clear(NULL);
      }
      i++;
      cursor_of_i++;
      NODE_ptr node_ch;
      int cmd_len = list_size(*cmd);
      if (cursor_of_i == cmd_len) {
        int display_size = char_utf8_display_with(c.value.character);
        cursor_index += display_size;
      } else {
        if ((node_ch = list_index_get(cursor_of_i, *cmd)) == NULL)
          continue;
        CharUTF8* ch = (CharUTF8*)node_ch->point;
        cursor_index += char_utf8_display_with(*ch);
      }
      scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
    }
    show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
  }
  terminal_raw_input_off();
  signal(SIGWINCH, SIG_DFL);
  return cmd;
error:
  terminal_raw_input_off();
  signal(SIGWINCH, SIG_DFL);
  return NULL;
}
