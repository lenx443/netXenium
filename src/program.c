#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "colors.h"
#include "commands.h"
#include "functions.h"
#include "history.h"
#include "list.h"
#include "logs.h"
#include "macros.h"
#include "program.h"
#include "properties.h"
#include "suggestion.h"
#include "terminal.h"

#define NAME "shell"

static term_size current_term_size = {0};

static void handle_winch(int sig) {
  if (!get_terminal_size(&current_term_size)) {
    log_add(NULL, ERROR, "SHELL", "Ocurrio un problema al obtener el tamaño de la terminal");
    log_show_and_clear(NULL);
  }
}

static void show_cmd(term_size prompt_end, const char *cmd, int cursor_index, int *scroll_offset) {
  int visible_cols = current_term_size.COLS - prompt_end.COLS - 2;
  int left_margin = 5;

  if (cursor_index < *scroll_offset + left_margin) {
    *scroll_offset = cursor_index - left_margin;
    if (*scroll_offset < 0) *scroll_offset = 0;
  }

  else if (cursor_index >= *scroll_offset + visible_cols) {
    *scroll_offset = cursor_index - visible_cols + 1;
  }

  const char *visible_cmd = cmd + *scroll_offset;
  int max_len = strlen(visible_cmd);
  if (max_len > visible_cols) max_len = visible_cols;

  printf("\033[%dG\033[K", prompt_end.COLS);
  printf("%.*s", max_len, visible_cmd);

  printf("\033[%dG", prompt_end.COLS + (cursor_index - *scroll_offset));
  fflush(stdout);
}

static int get_scroll(int cursor_index, int scroll_offset, const char *prompt) {
  int prompt_len = strip_ansi_escape_strlen(prompt);
  int cols = current_term_size.COLS - prompt_len;
  if (cursor_index < scroll_offset) {
    scroll_offset = cursor_index;
  } else if (cursor_index >= scroll_offset + cols) {
    scroll_offset = cursor_index - cols + 1;
  }
  return scroll_offset;
}

static int is_special_char(char c) { return c == '\'' || c == '"' || c == '$'; }

static int is_escaped(const char *cmd, int pos) {
  int backslashes = 0;
  while (pos > 0 && cmd[--pos] == '\\')
    backslashes++;
  return (backslashes % 2) != 0;
}

static int extract_money_token(const char *cmd, int start, char money, char **out) {
  int i = start + 1;
  char prop_name[PROP_NAME_LEN];
  int j = 0;

  while (isalnum(cmd[i]) || cmd[i] == '_') {
    if (j < PROP_NAME_LEN - 1) prop_name[j++] = cmd[i];
    i++;
  }
  prop_name[j] = '\0';
  prop_struct *prop_value = prop_reg_value(prop_name, *prop_register);
  if (!prop_value) {
    log_add(NULL, ERROR, "Command Parser", "Error al obtener el valor de la propiedad " AZUL "%s" RESET, prop_name);
    log_show_and_clear(NULL);
    *out = malloc(1);
    (*out)[0] = '\0';
    return i;
  }

  int out_size = strlen(prop_value->value) + 1;
  *out = malloc(out_size);
  strncpy(*out, prop_value->value, out_size - 1);
  (*out)[out_size - 1] = '\0';
  return i;
};

static int extract_quoted_token(const char *cmd, int start, char quote, char **out) {
  int i = start + 1;
  char buffer[1024];
  int j = 0;

  while (cmd[i] != '\0') {
    if (cmd[i] == quote && !is_escaped(cmd, i)) {
      buffer[j] = '\0';
      *out = malloc(j + 1);
      strncpy(*out, buffer, j);
      (*out)[j] = '\0';

      return i + 1;
    }

    if (cmd[i] == '\\' && is_special_char(cmd[i + 1])) {
      buffer[j++] = cmd[i + 1];
      i += 2;
    } else if (cmd[i] == '$' && !is_escaped(cmd, i)) {
      char *money;
      int new_pos = extract_money_token(cmd, i, cmd[i], &money);
      for (int k = 0; money[k]; k++) {
        buffer[j++] = money[k];
      }
      free(money);
      i = new_pos;
    } else {
      buffer[j++] = cmd[i++];
    }
  }

  *out = NULL;
  return start + 1;
}

static int extract_normal_token(const char *cmd, int start, char **out) {
  int i = start;
  char buffer[1024];
  int j = 0;
  while (cmd[i] != '\0' && cmd[i] != ' ') {
    if (cmd[i] == '\\' && is_special_char(cmd[i + 1])) {
      buffer[j++] = cmd[i + 1];
      i += 2;
    } else if ((cmd[i] == '\'' || cmd[i] == '"') && !is_escaped(cmd, i)) {
      char *quoted;
      i = extract_quoted_token(cmd, i, cmd[i], &quoted);
      if (!quoted) {
        log_add(NULL, ERROR, "Command Parser", "Comilla sin cerrar detectada");
        log_add(NULL, ERROR, "Command Parser", "{%s} <- Exprecion invalida", cmd);
        log_show_and_clear(NULL);
        free(*out);
        *out = NULL;
        program.closed = 1;
        return strlen(cmd);
      }
      for (int k = 0; quoted[k]; k++)
        buffer[j++] = quoted[k];
      free(quoted);
    } else if (cmd[i] == '$' && !is_escaped(cmd, i)) {
      char *money;
      i = extract_money_token(cmd, i, cmd[i], &money);
      for (int k = 0; money[k]; k++)
        buffer[j++] = money[k];
      free(money);
    } else {
      buffer[j++] = cmd[i++];
    }
  }
  buffer[j] = '\0';
  *out = strdup(buffer);
  return i;
}

int command_parser(char *cmd, ExecMode mode, SUGGEST_ptr *sugg, int sugg_pos) {
#define IF_SUGGEST if (mode == SUGGEST_MODE)
#define IF_EXEC if (mode == EXEC_MODE)
#define push_string(list, string)                                                                                      \
  if (!list_push_back_string_node(list, string)) {                                                                     \
    IF_SUGGEST {                                                                                                       \
      suggest_free(*sugg);                                                                                             \
      *sugg = NULL;                                                                                                    \
    }                                                                                                                  \
    list_free(args);                                                                                                   \
    return 0;                                                                                                          \
  }
  IF_SUGGEST suggest_clear(*sugg);
  char *comment_pos = strchr(cmd, '#');
  if (comment_pos != NULL) {
    IF_SUGGEST return 0;
    int comment_index = comment_pos - cmd;
    char *temp = strdup(cmd);
    memset(cmd, 0, CMDSIZ - 1);
    strncpy(cmd, temp, comment_index);
    cmd[comment_index] = '\0';
    free(temp);
  }
  IF_SUGGEST {
    if (sugg_pos < 0 || sugg_pos > (int)strlen(cmd)) { return 1; }

    int start = sugg_pos;
    while (start > 0 && cmd[start - 1] != ' ')
      start--;
    int end = sugg_pos;
    while (cmd[end] && cmd[end] != ' ')
      end++;

    int tok_len = end - start;
    if (tok_len <= 0) { return 1; }
    char *partial = strndup(cmd + start, tok_len);

    int is_first_token = (start == 0);

    char *money = strchr(partial, '$');
    if (money != NULL) {
      int money_pos = money - partial;
      NODE_ptr node = NULL;
      FOR_EACH(&node, *prop_register) {
        prop_struct *reg_val = (prop_struct *)node->point;
        if (strncmp(reg_val->key, money + 1, strlen(money) - 1) == 0) {
          char suggestion_cmd[CMDSIZ];
          snprintf(suggestion_cmd, sizeof(suggestion_cmd), "%.*s%.*s$%s%s", start, cmd, money_pos, partial,
                   reg_val->key, cmd + end);
          char *type = strdup("");
          for (int i = 0; map_types[i].key != OTHER; i++) {
            if (map_types[i].key == reg_val->type) {
              free(type);
              type = strdup(map_types[i].key_str);
            }
          }
          suggest_add(*sugg, reg_val->key, suggestion_cmd, type, COMMAND);
          free(type);
        }
      }
    } else if (is_first_token) {
      for (int i = 0; cmds_table[i] != NULL; i++) {
        if (strncmp(cmds_table[i]->name, partial, strlen(partial)) == 0) {
          char suggestion_cmd[CMDSIZ];
          snprintf(suggestion_cmd, sizeof(suggestion_cmd), "%.*s%s%s", start, cmd, cmds_table[i]->name, cmd + end);
          suggest_add(*sugg, cmds_table[i]->name, suggestion_cmd, cmds_table[i]->short_desc, COMMAND);
        }
      }
    }
    free(partial);
    return 1;
  }
  LIST_ptr args = list_new();
  {
    int i = 0;
    while (cmd[i] == ' ') {
      i++;
    }

    if (i > 0) {
      int j = 0;
      while (cmd[i]) {
        cmd[j++] = cmd[i++];
      }
      cmd[j] = '\0';
    }
    int len = 0;
    while (cmd[len] != '\0') {
      len++;
    }

    while (len > 0 && cmd[len - 1] == ' ') {
      len--;
    }
    cmd[len] = '\0';
  }
  int cmd_offset = 0;

  IF_EXEC while (cmd[cmd_offset] != '\0') {
    while (cmd[cmd_offset] == ' ')
      cmd_offset++;

    if (cmd[cmd_offset] == '\0') break;

    char *token = NULL;
    cmd_offset = extract_normal_token(cmd, cmd_offset, &token);
    if (token && token[0] != '\0') { push_string(args, token); }
    free(token);
  }

  int args_size = list_size(*args);
  if (args_size == 0) {
    list_free(args);
    return 1;
  }
  NODE_ptr cmd_node = list_index_get(0, *args);
  if (cmd_node == NULL) {
    list_free(args);
    return 0;
  }
  char *cmd_str = (char *)cmd_node->point;
  int matched = 0;
  for (int i = 0; cmds_table[i] != NULL; i++) {
    if (strcmp(cmds_table[i]->name, cmd_str) == 0) {
      IF_EXEC {
        if ((args_size - 1) < cmds_table[i]->args_len[0] || (args_size - 1) > cmds_table[i]->args_len[1]) {
          log_add(NULL, ERROR, "Command Parser", "\"{%s}\" <- Exprecion incorrecta", cmd);
          log_show_and_clear(NULL);
          program.return_code = 153;
          free(args);
          return 0;
        }
        program.return_code = cmds_table[i]->func(args);
        if (program.return_code == 153) {
          log_add(NULL, ERROR, "Command Parser", "\"{%s}\" <- Exprecion incorrecta", cmd);
          log_show_and_clear(NULL);
          free(args);
          return 0;
        }
      }
      matched = 1;
      break;
    }
  }
  IF_EXEC if (!matched) {
    log_add(NULL, ERROR, "Command Parser", "\"|%d>%s\" <- " AMARILLO "\'%s\'" RESET " no encontrado", strlen(cmd_str),
            cmd, cmd_str);

    log_show(NULL);
    log_clear(NULL);
    program.return_code = EXIT_FAILURE;
  }

  list_free(args);
  IF_EXEC return matched;
  return 1;
}

void load_script(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    log_add(NULL, ERROR, "Script-Loader", "No se pudo abrir el archivo: %s", filename);
    log_add_errno(NULL, ERROR, "Script-Loader");
    log_show_and_clear(NULL);
    return;
  }
  char line[CMDSIZ];
  while (fgets(line, CMDSIZ, fp)) {
    line[strcspn(line, "\n")] = '\0';
    if (!command_parser(line, EXEC_MODE, NULL, 0) || program.closed || program.return_code != 0) return;
  }
}

void shell_loop(char *name) {
#define default_promp(prompt)                                                                                          \
  sprintf(prompt, "[%s%d" RESET "] " AMARILLO "%s" RESET " > ", program.return_code == 0 ? VERDE : ROJO,               \
          program.return_code, name);

  printf(AZUL "NetXenium" RESET " (C) " AMARILLO "Lenx443 2024-2025" RESET "\n"
              "Type " VERDE "help" RESET " for more info\n");
  const char *home = getenv("HOME");
  if (home == NULL) {
    printf("No se encontro la variable entorno HOME\n");
    return;
  };
  char history_path[1024];
  snprintf(history_path, 1024, "%s/.xenium_history", home);
  history = history_new(history_path);

  while (1) {
    char cmd[CMDSIZ] = {0};
    int i = 0;
    int c;

    char prompt[100] = "\0";
    int prompt_position = prop_reg_search_key("PROMPT", *prop_register);
    if (prompt_position == -1) {
      default_promp(prompt);
      log_clear(NULL);
    } else {
      prop_struct *prompt_prop = prop_reg_value("PROMPT", *prop_register);
      if (prompt_prop->type != STRING) {
        default_promp(prompt);
      } else {
        strncpy(prompt, prompt_prop->value, 99);
        prompt[99] = '\0';
      }
    }
    printf("%s", prompt);
    fflush(stdout);

    int history_position = -1;
    int tab_pressed = 0;

    int prompt_len = strip_ansi_escape_strlen(prompt);
    int cursor_index = 0;
    int scroll_offset = 0;

    SUGGEST_ptr suggestions = suggest_new();
    int suggest_position = 0;

    terminal_raw_input_on();

    term_size prompt_end = get_cursor_position();

    handle_winch(SIGWINCH);
    signal(SIGWINCH, handle_winch);

    while (1) {
      c = read_raw_key();
      if (c == KEY_NULL) continue;

      if (c == KEY_EOF) {
        if (i != 0) continue;
        program.closed = 1;
        printf("\n");
        break;
      }
      if (c == KEY_ARROW_UP) {
        if (tab_pressed) {
          int suggest_size = list_size(*suggestions->suggestions);
          if (suggest_size == 0) continue;
          if (suggest_position == 0)
            suggest_position = suggest_size;
          else
            suggest_position--;
          suggest_hide(suggestions, prompt_len + 1 + cursor_index);
          suggest_show(suggestions, prompt_len + 1 + cursor_index, suggest_position);
        } else if ((history_position + 1) < history_size(*history)) {
          history_position++;
          HISTORY_struct *history_value = history_get(*history, history_position);
          if (history_value == NULL) continue;
          int previus_size = strlen(cmd);
          memset(cmd, 0, CMDSIZ);
          strncpy(cmd, history_value->command, CMDSIZ - 1);
          cmd[CMDSIZ - 1] = '\0';
          cursor_index = strlen(cmd);
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
          i = strlen(cmd);
          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
        }
        continue;
      }
      if (c == KEY_ARROW_DOWN) {
        if (tab_pressed) {
          int suggest_size = list_size(*suggestions->suggestions);
          if (suggest_size == 0) continue;
          if (suggest_position == suggest_size)
            suggest_position = 0;
          else
            suggest_position++;
          suggest_hide(suggestions, prompt_len + 1 + cursor_index);
          suggest_show(suggestions, prompt_len + 1 + cursor_index, suggest_position);
        } else {
          history_position--;
          if (history_position < 0) {
            int previus_size = strlen(cmd);
            memset(cmd, 0, CMDSIZ - 1);
            cursor_index = 0;
            scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
            i = 0;

            show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
            history_position = -1;
            continue;
          }
          HISTORY_struct *history_value = history_get(*history, history_position);
          if (history_value == NULL) continue;
          int previus_size = strlen(cmd);
          memset(cmd, 0, CMDSIZ - 1);
          strncpy(cmd, history_value->command, CMDSIZ - 1);
          cmd[CMDSIZ - 1] = '\0';
          cursor_index = strlen(cmd);
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
          i = strlen(cmd);
          cmd[i] = '\0';
          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
        }
        continue;
      }
      if (c == KEY_ARROW_LEFT) {
        if (cursor_index > 0) {
          cursor_index--;
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
          continue;
        }
        continue;
      }
      if (c == KEY_ARROW_RIGHT) {
        if (cursor_index < i) {
          cursor_index++;
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
          continue;
        }
        continue;
      }
      if (c == KEY_HOME) {
        cursor_index = 0;
        scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
        show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
        continue;
      }
      if (c == KEY_END) {
        cursor_index = i;
        scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
        show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
        continue;
      }
      if (c == '\n' || c == '\r') {
        if (tab_pressed && suggest_position > 0) {
          suggest_hide(suggestions, prompt_len + 1 + cursor_index);
          suggest_struct *sg_struct = suggest_get(suggestions, suggest_position);
          int previus_size = strlen(cmd);
          memset(cmd, 0, CMDSIZ);
          log_add(NULL, INFO, "SHELL", "sg_value: %s", sg_struct->sg_value);
          strncpy(cmd, sg_struct->sg_value, CMDSIZ);
          cmd[CMDSIZ - 1] = '\0';
          cursor_index = strlen(cmd);
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
          i = strlen(cmd);
          cmd[i] = '\0';

          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
          suggest_clear(suggestions);
          tab_pressed = 0;
          continue;
        }
        printf("\n");
        time_t tim = time(NULL);
        if (tim == (time_t)-1) break;
        if (cmd[0] != '\0') {
          HISTORY_struct *previus_history = history_get(*history, 0);
          if (previus_history == NULL || strcmp(cmd, previus_history->command) != 0) {
            HISTORY_struct new_history_line;
            new_history_line.time_stamp = (long)tim;
            strncpy(new_history_line.command, cmd, CMDSIZ - 1);
            new_history_line.command[CMDSIZ - 1] = '\0';
            history_push_line(history, new_history_line);
          }
        }
        tab_pressed = 0;
        history_position = -1;
        break;
      }
      if (c == '\t') {
        if (!tab_pressed) {
          suggest_position = 0;
          if (strlen(cmd) == 0) continue;
          command_parser(cmd, SUGGEST_MODE, &suggestions, cursor_index);
          if (suggestions == NULL) continue;

          int results_size = list_size(*suggestions->suggestions);
          if (results_size == 0) continue;

          if (results_size == 1) {
            suggest_struct *sugg = suggest_get(suggestions, 1);
            if (sugg == NULL) { continue; }

            if (strcmp(cmd, sugg->sg_value) != 0) {
              int previus_size = strlen(cmd);
              memset(cmd, 0x00, CMDSIZ - 1);
              strncpy(cmd, sugg->sg_value, CMDSIZ - 1);
              cmd[CMDSIZ - 1] = '\0';

              cursor_index = strlen(cmd);
              scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);
              i = strlen(cmd);

              show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
              tab_pressed = 0;
              continue;
            }
            tab_pressed = 0;
            continue;
          }
          suggest_hide(suggestions, prompt_len + 1 + cursor_index);
          printf("\r\n");
          suggest_show(suggestions, prompt_len + 1 + cursor_index, suggest_position);
          tab_pressed = 1;
          continue;
        } else {
          int suggs_len = list_size(*suggestions->suggestions);
          if (suggest_position < suggs_len)
            suggest_position++;
          else
            suggest_position = 0;
          suggest_hide(suggestions, prompt_len + 1 + cursor_index);
          suggest_show(suggestions, prompt_len + 1 + cursor_index, suggest_position);
        }
        continue;
      }
      if (c == KEY_BACKSPACE) {
        if (cursor_index > 0) {
          for (int j = cursor_index - 1; j < i; j++) {
            cmd[j] = cmd[j + 1];
          }
          i--;
          cursor_index--;
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);

          if (tab_pressed) {
            if (strlen(cmd) == 0) {
              suggest_hide(suggestions, prompt_len + 1 + cursor_index);
              suggest_position = 0;
              tab_pressed = 0;
            }
          }

          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
          continue;
        }
        continue;
      } else if (c == KEY_DELETE) {
        if (cursor_index < i) {
          for (int j = cursor_index; j < i - 1; j++) {
            cmd[j] = cmd[j + 1];
          }
          cmd[i - 1] = '\0';
          i--;
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);

          if (tab_pressed) {
            suggest_hide(suggestions, prompt_len + 1 + cursor_index);
            suggest_position = 0;
            tab_pressed = 0;
          }

          show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
          continue;
        }
        continue;
      } else {
        if (i < sizeof(cmd) - 1) {
          for (int j = i; j > cursor_index; j--) {
            cmd[j] = cmd[j - 1];
          }
          cmd[cursor_index] = (char)c;
          i++;
          cursor_index++;
          scroll_offset = get_scroll(cursor_index, scroll_offset, prompt);

          if (tab_pressed) {
            suggest_hide(suggestions, prompt_len + 1 + cursor_index);
            command_parser(cmd, SUGGEST_MODE, &suggestions, cursor_index);
            int suggs_len = list_size(*suggestions->suggestions);
            if (suggs_len != 0) {
              if (suggs_len < suggest_position) suggest_position = 0;
              suggest_show(suggestions, prompt_len + 1 + cursor_index, suggest_position);
            } else {
              suggest_position = 0;
              tab_pressed = 0;
            }
          }
        }
      }
      show_cmd(prompt_end, cmd, cursor_index, &scroll_offset);
    }
    terminal_raw_input_off();
    signal(SIGWINCH, SIG_DFL);
    suggest_free(suggestions);

    command_parser(cmd, EXEC_MODE, NULL, 0);

    if (program.closed) break;
  }
  history_save(*history);
  history_free(history);
}

Program_State program = {
    0,
    EXIT_SUCCESS,
    EXIT_SUCCESS,
};
const Command *cmds_table[] = {
    &cmd_help,  &cmd_exit,          &cmd_new,     &cmd_del,       &cmd_get,   &cmd_set, &cmd_echo,
    &cmd_input, &cmd_clear_history, &cmd_resolve, &cmd_arp_spoof, &cmd_iface, NULL,
};
HISTORY_ptr history = NULL;
