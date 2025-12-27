#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "lexer.h"
#include "source_file.h"
#include "xen_cstrings.h"
#include "xen_life.h"
#include "xen_typedefs.h"

static inline Xen_size_t advance(Lexer* lexer) {
  lexer->column++;
  return lexer->pos++;
}

static inline Xen_size_t xadvance(Lexer* lexer) {
  lexer->column++;
  return ++lexer->pos;
}

static inline void token_start(Lexer* lexer) {
  lexer->start_line = lexer->line;
  lexer->start_column = lexer->column;
}

static inline Lexer_Token Token(Lexer* lexer, Lexer_Token_Type type,
                                Xen_c_string_t text, Xen_size_t size) {
  if (size >= LXR_TOKEN_SIZE) {
    size = LXR_TOKEN_SIZE - 1;
  }
  Lexer_Token token = {0};
  token.tkn_type = type;
  strncpy(token.tkn_text, text, size);
  token.tkn_text[size] = '\0';
  token.sta = (Xen_Source_Address){lexer->sf_id, lexer->start_line,
                                   lexer->start_column};
  return token;
}

void skip_whitespace(Lexer* lexer) {
  Xen_Source_File* sf = (*xen_globals->source_table)->st_files[lexer->sf_id];
  while (1) {
    char c = sf->sf_content[lexer->pos];
    if (c == ' ' || c == '\t') {
      advance(lexer);
    } else if (c == '#') {
      while (sf->sf_content[lexer->pos] && sf->sf_content[lexer->pos] != '\n')
        advance(lexer);
    } else {
      break;
    }
  }
}

Lexer_Token lexer_next_token(Lexer* lexer) {
  skip_whitespace(lexer);
  Xen_Source_File* sf = (*xen_globals->source_table)->st_files[lexer->sf_id];

  Lexer_Token token = Token(lexer, TKN_EOF, "", 0);

  char c = sf->sf_content[lexer->pos];
  if (c == '\0') {
    token_start(lexer);
    token = Token(lexer, TKN_EOF, "<EOF>", 5);
  } else if (c == '\n') {
    token_start(lexer);
    advance(lexer);
    lexer->line++;
    lexer->column = 1;
    token = Token(lexer, TKN_NEWLINE, "<New-Line>", 10);
  } else if (c == ';' || c == '\n' || c == '\r') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_NEWLINE, "<New-Line>", 10);
  } else if (isalpha(c) || c == '_') {
    token_start(lexer);
    size_t start = lexer->pos;
    while (isalnum(sf->sf_content[lexer->pos]) ||
           sf->sf_content[lexer->pos] == '_')
      advance(lexer);
    size_t len = lexer->pos - start;
    char buffer[len + 1];
    strncpy(buffer, sf->sf_content + start, len);
    buffer[len] = '\0';
    if (strcmp(buffer, "if") == 0 || strcmp(buffer, "elif") == 0 ||
        strcmp(buffer, "else") == 0 || strcmp(buffer, "while") == 0 ||
        strcmp(buffer, "for") == 0 || strcmp(buffer, "in") == 0 ||
        strcmp(buffer, "break") == 0 || strcmp(buffer, "continue") == 0 ||
        strcmp(buffer, "return") == 0 || strcmp(buffer, "implement") == 0 ||
        strcmp(buffer, "throw") == 0 || strcmp(buffer, "try") == 0 ||
        strcmp(buffer, "catch") == 0) {
      token = Token(lexer, TKN_KEYWORD, sf->sf_content + start, len);
    } else if (strcmp(buffer, "has") == 0) {
      token = Token(lexer, TKN_HAS, sf->sf_content + start, len);
    } else if (strcmp(buffer, "not") == 0) {
      token = Token(lexer, TKN_NOT, sf->sf_content + start, len);
    } else if (strcmp(buffer, "and") == 0) {
      token = Token(lexer, TKN_AND, sf->sf_content + start, len);
    } else if (strcmp(buffer, "or") == 0) {
      token = Token(lexer, TKN_OR, sf->sf_content + start, len);
    } else {
      token = Token(lexer, TKN_IDENTIFIER, sf->sf_content + start, len);
    }
  } else if (c == '$') {
    token_start(lexer);
    advance(lexer);
    size_t start = lexer->pos;
    while (isalnum(sf->sf_content[lexer->pos]) ||
           sf->sf_content[lexer->pos] == '_' ||
           sf->sf_content[lexer->pos] == '$')
      advance(lexer);
    size_t len = lexer->pos - start;
    token = Token(lexer, TKN_PROPERTY, sf->sf_content + start, len);
  } else if (c == '"') {
    token_start(lexer);
    advance(lexer);

    char buffer[1024];
    size_t bpos = 0;

    while (sf->sf_content[lexer->pos] != '"') {
      char ch = sf->sf_content[lexer->pos];

      if (ch == '\0') {
        advance(lexer);
        token = Token(lexer, TKN_UNDEFINED, "<UNDEF>", 7);
        return token;
      }

      if (ch == '\\') {
        advance(lexer);
        ch = sf->sf_content[lexer->pos];
        if (ch == '\0')
          break;

        switch (ch) {
        case 'n':
          buffer[bpos++] = '\n';
          break;
        case 'r':
          buffer[bpos++] = '\r';
          break;
        case 't':
          buffer[bpos++] = '\t';
          break;
        case 'v':
          buffer[bpos++] = '\v';
          break;
        case 'f':
          buffer[bpos++] = '\f';
          break;
        case 'a':
          buffer[bpos++] = '\a';
          break;
        case '\\':
          buffer[bpos++] = '\\';
          break;
        case '\'':
          buffer[bpos++] = '\'';
          break;
        case '\"':
          buffer[bpos++] = '\"';
          break;
        case '?':
          buffer[bpos++] = '?';
          break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7': {
          int val = ch - '0';
          int count = 1;
          while (count < 3 && sf->sf_content[lexer->pos + 1] >= '0' &&
                 sf->sf_content[lexer->pos + 1] <= '7') {
            val = val * 8 + (sf->sf_content[xadvance(lexer)] - '0');
            count++;
          }
          buffer[bpos++] = (char)val;
        } break;

        case 'x': {
          int val = 0;
          while (isxdigit((unsigned char)sf->sf_content[lexer->pos + 1])) {
            char c2 = sf->sf_content[xadvance(lexer)];
            val = val * 16 +
                  (isdigit(c2) ? c2 - '0'
                               : (isupper(c2) ? c2 - 'A' + 10 : c2 - 'a' + 10));
          }
          buffer[bpos++] = (char)val;
        } break;

        case 'u':
        case 'U': {
          int digits = (ch == 'u') ? 4 : 8;
          unsigned int code = 0;
          for (int i = 0; i < digits; i++) {
            char c2 = sf->sf_content[lexer->pos + 1];
            if (!isxdigit((unsigned char)c2))
              break;
            advance(lexer);
            code = code * 16 + (isdigit(c2) ? c2 - '0'
                                            : (isupper(c2) ? c2 - 'A' + 10
                                                           : c2 - 'a' + 10));
          }
          if (code < 0x80) {
            buffer[bpos++] = (char)code;
          } else if (code < 0x800) {
            buffer[bpos++] = 0xC0 | (code >> 6);
            buffer[bpos++] = 0x80 | (code & 0x3F);
          } else if (code < 0x10000) {
            buffer[bpos++] = 0xE0 | (code >> 12);
            buffer[bpos++] = 0x80 | ((code >> 6) & 0x3F);
            buffer[bpos++] = 0x80 | (code & 0x3F);
          } else {
            buffer[bpos++] = 0xF0 | (code >> 18);
            buffer[bpos++] = 0x80 | ((code >> 12) & 0x3F);
            buffer[bpos++] = 0x80 | ((code >> 6) & 0x3F);
            buffer[bpos++] = 0x80 | (code & 0x3F);
          }
        } break;

        default:
          buffer[bpos++] = ch;
          break;
        }
      } else {
        buffer[bpos++] = ch;
      }

      advance(lexer);
    }

    buffer[bpos] = '\0';
    token = Token(lexer, TKN_STRING, buffer, Xen_CString_Len(buffer));
    advance(lexer);
  } else if (c == '\'') {
    token_start(lexer);
    advance(lexer);

    char buffer[1024];
    size_t bpos = 0;

    while (sf->sf_content[lexer->pos] != '\'') {
      char ch = sf->sf_content[lexer->pos];

      if (ch == '\0') {
        advance(lexer);
        token = Token(lexer, TKN_UNDEFINED, "<UNDEF>", 7);
        return token;
      }

      if (ch == '\\') {
        advance(lexer);
        ch = sf->sf_content[lexer->pos];
        if (ch == '\0')
          break;

        switch (ch) {
        case 'n':
          buffer[bpos++] = '\n';
          break;
        case 'r':
          buffer[bpos++] = '\r';
          break;
        case 't':
          buffer[bpos++] = '\t';
          break;
        case 'v':
          buffer[bpos++] = '\v';
          break;
        case 'f':
          buffer[bpos++] = '\f';
          break;
        case 'a':
          buffer[bpos++] = '\a';
          break;
        case '\\':
          buffer[bpos++] = '\\';
          break;
        case '\'':
          buffer[bpos++] = '\'';
          break;
        case '\"':
          buffer[bpos++] = '\"';
          break;
        case '?':
          buffer[bpos++] = '?';
          break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7': {
          int val = ch - '0';
          int count = 1;
          while (count < 3 && sf->sf_content[lexer->pos + 1] >= '0' &&
                 sf->sf_content[lexer->pos + 1] <= '7') {
            val = val * 8 + (sf->sf_content[xadvance(lexer)] - '0');
            count++;
          }
          buffer[bpos++] = (char)val;
        } break;

        case 'x': {
          int val = 0;
          while (isxdigit((unsigned char)sf->sf_content[lexer->pos + 1])) {
            char c2 = sf->sf_content[xadvance(lexer)];
            val = val * 16 +
                  (isdigit(c2) ? c2 - '0'
                               : (isupper(c2) ? c2 - 'A' + 10 : c2 - 'a' + 10));
          }
          buffer[bpos++] = (char)val;
        } break;

        case 'u':
        case 'U': {
          int digits = (ch == 'u') ? 4 : 8;
          unsigned int code = 0;
          for (int i = 0; i < digits; i++) {
            char c2 = sf->sf_content[lexer->pos + 1];
            if (!isxdigit((unsigned char)c2))
              break;
            advance(lexer);
            code = code * 16 + (isdigit(c2) ? c2 - '0'
                                            : (isupper(c2) ? c2 - 'A' + 10
                                                           : c2 - 'a' + 10));
          }
          if (code < 0x80) {
            buffer[bpos++] = (char)code;
          } else if (code < 0x800) {
            buffer[bpos++] = 0xC0 | (code >> 6);
            buffer[bpos++] = 0x80 | (code & 0x3F);
          } else if (code < 0x10000) {
            buffer[bpos++] = 0xE0 | (code >> 12);
            buffer[bpos++] = 0x80 | ((code >> 6) & 0x3F);
            buffer[bpos++] = 0x80 | (code & 0x3F);
          } else {
            buffer[bpos++] = 0xF0 | (code >> 18);
            buffer[bpos++] = 0x80 | ((code >> 12) & 0x3F);
            buffer[bpos++] = 0x80 | ((code >> 6) & 0x3F);
            buffer[bpos++] = 0x80 | (code & 0x3F);
          }
        } break;

        default:
          buffer[bpos++] = ch;
          break;
        }
      } else {
        buffer[bpos++] = ch;
      }

      advance(lexer);
    }

    buffer[bpos] = '\0';
    token = Token(lexer, TKN_STRING, buffer, Xen_CString_Len(buffer));
    advance(lexer);
  } else if (isdigit(c)) {
    token_start(lexer);
    if (c == '0') {
      advance(lexer);
      if (sf->sf_content[lexer->pos] == 'x' ||
          sf->sf_content[lexer->pos] == 'X') {
        advance(lexer);
        size_t start = lexer->pos;
        while (isxdigit(sf->sf_content[lexer->pos])) {
          advance(lexer);
        }
        size_t len = lexer->pos - start;
        char buffer[len + 4];
        strcpy(buffer, "0x");
        if (len)
          strncpy(buffer + 2, sf->sf_content + start, len);
        token = Token(lexer, TKN_NUMBER, buffer, Xen_CString_Len(buffer));
      } else if (sf->sf_content[lexer->pos] == 'b' ||
                 sf->sf_content[lexer->pos] == 'B') {
        advance(lexer);
        size_t start = lexer->pos;
        while (sf->sf_content[lexer->pos] == '0' ||
               sf->sf_content[lexer->pos] == '1') {
          advance(lexer);
        }
        size_t len = lexer->pos - start;
        char buffer[len + 4];
        strcpy(buffer, "0b");
        if (len)
          strncpy(buffer + 2, sf->sf_content + start, len);
        token = Token(lexer, TKN_NUMBER, buffer, Xen_CString_Len(buffer));
      } else if (sf->sf_content[lexer->pos] == 'o' ||
                 sf->sf_content[lexer->pos] == 'O') {
        advance(lexer);
        size_t start = lexer->pos;
        while (sf->sf_content[lexer->pos] >= '0' &&
               sf->sf_content[lexer->pos] < '8') {
          advance(lexer);
        }
        size_t len = lexer->pos - start;
        char buffer[len + 4];
        strcpy(buffer, "0o");
        if (len)
          strncpy(buffer + 2, sf->sf_content + start, len);
        token = Token(lexer, TKN_NUMBER, buffer, Xen_CString_Len(buffer));
      } else if (sf->sf_content[lexer->pos] >= '0' &&
                 sf->sf_content[lexer->pos] <= '8') {
        size_t start = lexer->pos;
        while (sf->sf_content[lexer->pos] >= '0' &&
               sf->sf_content[lexer->pos] < '8') {
          advance(lexer);
        }
        size_t len = lexer->pos - start;
        char buffer[len + 3];
        strcpy(buffer, "0");
        if (len)
          strncpy(buffer + 1, sf->sf_content + start, len);
        token = Token(lexer, TKN_NUMBER, buffer, Xen_CString_Len(buffer));
      } else if (sf->sf_content[lexer->pos] == '.') {
        advance(lexer);
        Xen_size_t start = lexer->pos;
        while (isdigit(sf->sf_content[lexer->pos])) {
          advance(lexer);
        }
        size_t len = lexer->pos - start;
        char buffer[len + 4];
        strcpy(buffer, "0.");
        if (len)
          strncpy(buffer + 2, sf->sf_content + start, len);
        token = Token(lexer, TKN_NUMBER, buffer, Xen_CString_Len(buffer));
      } else {
        token = Token(lexer, TKN_NUMBER, "0", 1);
      }
    } else {
      size_t start = lexer->pos;
      int seen_digit = 0;
      int seen_dot = 0;

      while (1) {
        char ch = sf->sf_content[lexer->pos];
        if (isdigit(ch)) {
          seen_digit = 1;
          advance(lexer);
        } else if (ch == '.' && !seen_dot) {
          seen_dot = 1;
          advance(lexer);
        } else {
          break;
        }
      }

      if (!seen_digit) {
        token = Token(lexer, TKN_UNDEFINED, "<UNDEF>", 7);
        return token;
      }

      size_t len = lexer->pos - start;
      token = Token(lexer, TKN_NUMBER, sf->sf_content + start, len);
    }
  } else if (c == '{') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_LBRACE, "{", 1);
  } else if (c == '}') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_RBRACE, "}", 1);
  } else if (c == '=') {
    token_start(lexer);
    advance(lexer);
    if (sf->sf_content[lexer->pos] == '=') {
      advance(lexer);
      token = Token(lexer, TKN_EQ, "==", 2);
    } else if (sf->sf_content[lexer->pos] == '>') {
      advance(lexer);
      token = Token(lexer, TKN_BLOCK, "=>", 2);
    } else {
      token = Token(lexer, TKN_ASSIGNMENT, "=", 1);
    }
  } else if (c == '(') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_LPARENT, "(", 1);
  } else if (c == ')') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_RPARENT, ")", 1);
  } else if (c == '[') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_LBRACKET, "[", 1);
  } else if (c == ']') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_RBRACKET, "]", 1);
  } else if (c == '\\') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_ATTR, "\\", 1);
  } else if (c == ',') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_COMMA, ",", 1);
  } else if (c == ':') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_COLON, ":", 1);
  } else if (c == '?') {
    token_start(lexer);
    advance(lexer);
    if (sf->sf_content[lexer->pos] == '?') {
      advance(lexer);
      token = Token(lexer, TKN_DOUBLE_QUESTION, "??", 2);
    } else {
      token = Token(lexer, TKN_QUESTION, "?", 1);
    }
  } else if (c == '+') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_ADD, "+", 1);
  } else if (c == '-') {
    token_start(lexer);
    advance(lexer);
    if (sf->sf_content[lexer->pos] == '>') {
      advance(lexer);
      token = Token(lexer, TKN_ARROW, "->", 2);
    } else {
      token = Token(lexer, TKN_MINUS, "-", 1);
    }
  } else if (c == '*') {
    token_start(lexer);
    advance(lexer);
    if (sf->sf_content[lexer->pos] == '*') {
      advance(lexer);
      token = Token(lexer, TKN_POW, "**", 2);
    } else {
      token = Token(lexer, TKN_MUL, "*", 1);
    }
  } else if (c == '/') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_DIV, "/", 1);
  } else if (c == '%') {
    token_start(lexer);
    advance(lexer);
    token = Token(lexer, TKN_MOD, "%", 1);
  } else if (c == '<') {
    token_start(lexer);
    advance(lexer);
    if (sf->sf_content[lexer->pos] == '=') {
      advance(lexer);
      token = Token(lexer, TKN_LE, "<=", 2);
    } else {
      token = Token(lexer, TKN_LT, "<", 1);
    }
  } else if (c == '>') {
    token_start(lexer);
    advance(lexer);
    if (sf->sf_content[lexer->pos] == '=') {
      advance(lexer);
      token = Token(lexer, TKN_GE, ">=", 2);
    } else {
      token = Token(lexer, TKN_GT, ">", 1);
    }
  } else if (c == '!') {
    token_start(lexer);
    advance(lexer);
    if (sf->sf_content[lexer->pos] == '=') {
      advance(lexer);
      token = Token(lexer, TKN_NE, "!=", 2);
    } else {
      token = Token(lexer, TKN_UNDEFINED, "!", 1);
    }
  } else {
    token_start(lexer);
    token = Token(lexer, TKN_UNDEFINED, &sf->sf_content[lexer->pos], 1);
    advance(lexer);
  }
  return token;
}
