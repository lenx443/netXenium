#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "colors.h"
#include "list.h"
#include "logs.h"
#include "macros.h"

static char *__log_type(log_types type) {
  switch (type) {
  case ERROR: return "ERROR";
  case WARNING: return "WARNING";
  case INFO: return "INFO";
  default: return "Undefined";
  }
}

static char *__log_type_color(log_types type) {
  switch (type) {
  case ERROR: return ROJO;
  case WARNING: return AMARILLO;
  case INFO: return VERDE;
  default: return RESET;
  }
}

static void strip_ansi_escape(char *str) {
  char *src = str;
  char *dst = str;
  while (*src) {
    if (*src == '\x1B' && *(src + 1) == '[') {
      src += 2;
      while (*src && !((*src >= '@' && *src <= '~'))) {
        src++;
      }
      if (*src) src++;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

int is_loged(LIST_ptr object, char *match) {
  LIST_ptr log = object ? object : global_logs;

  NODE_ptr node = NULL;
  FOR_EACH(&node, *log) {
    log_struct *log_value = (log_struct *)node->point;
    if (strcmp(log_value->source, match) == 0) return 1;
  }
  return 0;
}

void log_add(LIST_ptr object, log_types type, char *source, char *format, ...) {
  LIST_ptr log = object ? object : global_logs;

  va_list args;
  char logbuf[BUFSIZ];
  char clean_format[BUFSIZ];
  int sel_pos[128][2];
  int sel_count = 0;
  int i = 0, j = 0;

  while (format[i] && j < BUFSIZ - 1) {
    if (format[i] == '|' && isdigit(format[i + 1])) {
      int k = i + 1, n = 0;
      while (isdigit(format[k])) {
        n = n * 10 + (format[k] - '0');
        k++;
      }
      if (format[k] == '>') {
        int written = snprintf(clean_format + j, BUFSIZ - j - 1, "|%d>", n);
        j += written;
        i = k + 1;
        continue;
      }
    }

    if (format[i] == '{' && sel_count < 128) {
      int start = j;
      i++;
      while (format[i] && format[i] != '}' && j < BUFSIZ - 1)
        clean_format[j++] = format[i++];
      if (format[i] == '}') i++;
      sel_pos[sel_count][0] = start;
      sel_pos[sel_count][1] = j;
      sel_count++;
    } else {
      clean_format[j++] = format[i++];
    }
  }
  clean_format[j] = '\0';

  va_start(args, format);
  int offset = vsnprintf(logbuf, BUFSIZ - 1, clean_format, args);
  va_end(args);

  if (offset >= BUFSIZ - 1) return;

  if (strchr(format, '|') || sel_count > 0) {
    offset += snprintf(logbuf + offset, BUFSIZ - offset - 1, "\n");
  }

  int visual_map[BUFSIZ];
  int visual_len = 0, in_escape = 0;
  for (int real = 0; real < offset && visual_len < BUFSIZ - 1; real++) {
    visual_map[real] = visual_len;
    if (!in_escape && logbuf[real] == '\x1b')
      in_escape = 1;
    else if (in_escape && logbuf[real] == 'm')
      in_escape = 0;
    else if (!in_escape)
      visual_len++;
  }
  visual_map[offset] = visual_len;

  char underline[BUFSIZ] = {0};
  memset(underline, ' ', visual_len);
  underline[visual_len] = '\0';

  char *cursor = logbuf;
  while ((cursor = strchr(cursor, '|')) != NULL) {
    char *start = cursor;
    cursor++;

    int n = 0;
    while (isdigit(*cursor)) {
      n = n * 10 + (*cursor - '0');
      cursor++;
    }

    if (*cursor == '>') {
      int start_idx = start - logbuf;
      int vis_start = visual_map[start_idx];
      int vis_end = vis_start + n;

      for (int u = vis_start; u < vis_end && u < BUFSIZ; u++) {
        underline[u] = '^';
      }

      size_t len = strlen(cursor + 1);
      memmove(start, cursor + 1, len + 1);
      offset -= (cursor + 1 - start);
      cursor = start;
    } else {
      cursor = start + 1;
    }
  }

  va_start(args, format);
  for (int s = 0; s < sel_count; s++) {
    int len = sel_pos[s][1] - sel_pos[s][0];
    if (len <= 0 || len >= BUFSIZ) continue;

    char temp_fmt[BUFSIZ], temp_arg_text[BUFSIZ];
    strncpy(temp_fmt, &clean_format[sel_pos[s][0]], len);
    temp_fmt[len] = '\0';

    vsnprintf(temp_arg_text, BUFSIZ, temp_fmt, args);
    char *found = strstr(logbuf, temp_arg_text);
    if (found) {
      int real_start = found - logbuf;
      int real_end = real_start + strlen(temp_arg_text);

      int vis_start = visual_map[real_start];
      int vis_end = visual_map[real_end];

      if (vis_start >= 0 && vis_end > vis_start && vis_end < BUFSIZ) {
        for (int u = vis_start; u < vis_end && u < BUFSIZ; u++) {
          underline[u] = '^';
        }
      }
    }
  }
  va_end(args);

  if (strchr(underline, '^')) {
    if (offset + strlen(underline) < BUFSIZ - 1) {
      snprintf(logbuf + offset, BUFSIZ - offset - 1, "%s", underline);
    }
  }

  time_t current_time;
  if (time(&current_time) == (time_t)-1) return;

  log_struct new_log_struct;
  strncpy(new_log_struct.source, source, LOG_SOURCE_LEN - 1);
  new_log_struct.source[LOG_SOURCE_LEN - 1] = '\0';
  strncpy(new_log_struct.content, logbuf, LOG_CONTENT_LEN - 1);
  new_log_struct.content[LOG_CONTENT_LEN - 1] = '\0';
  new_log_struct.level = type;
  new_log_struct.tm = current_time;
  list_push_back(log, &new_log_struct, sizeof(log_struct));
}

void log_add_errno(LIST_ptr log, log_types type, char *source) {
  log_add(log, type, source, "errno(%d) %s", errno, strerror(errno));
}

void log_get(LIST_ptr object, char *log_out, size_t bufsiz) {
  LIST_ptr log = object ? object : global_logs;

  int offset = 0;
  char logbuf[bufsiz];
  logbuf[0] = '\0';

  char previus_source[LOG_SOURCE_LEN];
  log_types previus_log_type;

  NODE_ptr log_node = NULL;
  FOR_EACH(&log_node, *log) {
    int written = 0;
    log_struct *log_value = (log_struct *)log_node->point;
    if (strcmp(previus_source, log_value->source) != 0 ||
        previus_log_type != log_value->level) {
      struct tm *tm_info = localtime(&log_value->tm);
      if (!tm_info) continue;
      written =
          snprintf(logbuf + offset, bufsiz - offset,
                   "\n[%s%s" RESET "] [" VERDE "%d/%d/%d %d:%d:%d" RESET "] %s\n",
                   __log_type_color(log_value->level), __log_type(log_value->level),
                   tm_info->tm_mday, tm_info->tm_mon, tm_info->tm_year, tm_info->tm_hour,
                   tm_info->tm_min, tm_info->tm_sec, log_value->source);
      if (written < 0 || (size_t)written >= bufsiz - offset) break;
      offset += written;
      strncpy(previus_source, log_value->source, LOG_SOURCE_LEN);
      previus_source[LOG_SOURCE_LEN - 1] = '\0';
      previus_log_type = log_value->level;
    }

    written = snprintf(logbuf + offset, bufsiz - offset, "%s\n", log_value->content);
    if (written < 0 || (size_t)written >= bufsiz - offset) break;
    offset += written;
  }
  strncpy(log_out, logbuf, bufsiz - 1);
  log_out[bufsiz - 1] = '\0';
}

void log_show(LIST_ptr object) {
  LIST_ptr log = object ? object : global_logs;
  if (!log->head) return;
  char logs_text[65536];
  log_get(log, logs_text, 65536);
  printf("%s\n", logs_text);
}

void log_show_and_clear(LIST_ptr log) {
  log_show(log);
  log_clear(log);
}

void log_file_save(LIST_ptr object, char *filename) {
  LIST_ptr log = object ? object : global_logs;
  FILE *log_file = fopen(filename, "w");
  if (!log_file) return;
  char logs_text[65536];
  log_get(log, logs_text, 65536);
  strip_ansi_escape(logs_text);
  fprintf(log_file, "%s", logs_text);
  fclose(log_file);
}

void log_clear(LIST_ptr object) {
  LIST_ptr log = object ? object : global_logs;
  list_clear(log);
}

void log_free(LIST_ptr object) {
  LIST_ptr log = object ? object : global_logs;
  log_clear(log);
  list_free(log);
  log = NULL;
}

LIST_ptr global_logs;
