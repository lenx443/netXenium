#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "lexer.h"

void skip_whitespace(Lexer *lexer) {
  while (1) {
    char c = lexer->src[lexer->pos];
    if (c == ' ' || c == '\t') {
      lexer->pos++;
    } else if (c == '#') {
      while (lexer->src[lexer->pos] && lexer->src[lexer->pos] != '\n')
        lexer->pos++;
    } else {
      break;
    }
  }
}

Lexer_Token lexer_next_token(Lexer *lexer) {
  skip_whitespace(lexer);

  Lexer_Token token = {0};
  token.tkn_type = TKN_EOF;
  token.tkn_text[0] = '\n';

  char c = lexer->src[lexer->pos];
  if (c == '\0') {
    token.tkn_type = TKN_EOF;
    strcpy(token.tkn_text, "<EOF>");
  } else if (c == ';' || c == '\n' || c == '\r') {
    lexer->pos++;
    token.tkn_type = TKN_NEWLINE;
    strcpy(token.tkn_text, "<New-Line>");
  } else if (isalpha(c) || c == '_') {
    size_t start = lexer->pos;
    while (isalnum(lexer->src[lexer->pos]) || lexer->src[lexer->pos] == '_')
      lexer->pos++;
    size_t len = lexer->pos - start;
    strncpy(token.tkn_text, lexer->src + start, len);
    token.tkn_text[len] = '\0';
    token.tkn_type = TKN_IDENTIFIER;
  } else if (c == '$') {
    lexer->pos++;
    size_t start = lexer->pos;
    while (isalnum(lexer->src[lexer->pos]) || lexer->src[lexer->pos] == '_')
      lexer->pos++;
    size_t len = lexer->pos - start;
    strncpy(token.tkn_text, lexer->src + start, len);
    token.tkn_text[len] = '\0';
    token.tkn_type = TKN_PROPERTY;
  } else if (c == '"') {
    lexer->pos++;

    char buffer[1024]; // temporal para almacenar string procesada
    size_t bpos = 0;

    while (lexer->src[lexer->pos] != '"') {
      char ch = lexer->src[lexer->pos];

      if (ch == '\0') {
        lexer->pos++;
        token.tkn_type = TKN_UNDEFINED;
        strcpy(token.tkn_text, "<UNDEF>");
        return token;
      }

      if (ch == '\\') { // secuencia de escape
        lexer->pos++;
        ch = lexer->src[lexer->pos];
        if (ch == '\0') break;

        switch (ch) {
        case 'n': buffer[bpos++] = '\n'; break;
        case 'r': buffer[bpos++] = '\r'; break;
        case 't': buffer[bpos++] = '\t'; break;
        case 'v': buffer[bpos++] = '\v'; break;
        case 'f': buffer[bpos++] = '\f'; break;
        case 'a': buffer[bpos++] = '\a'; break;
        case '\\': buffer[bpos++] = '\\'; break;
        case '\'': buffer[bpos++] = '\''; break;
        case '\"': buffer[bpos++] = '\"'; break;
        case '?': buffer[bpos++] = '?'; break;

        // octales \ooo
        case '0' ... '7': {
          int val = ch - '0';
          int count = 1;
          while (count < 3 && lexer->src[lexer->pos + 1] >= '0' &&
                 lexer->src[lexer->pos + 1] <= '7') {
            val = val * 8 + (lexer->src[++lexer->pos] - '0');
            count++;
          }
          buffer[bpos++] = (char)val;
        } break;

        // hexadecimales \xhh
        case 'x': {
          int val = 0;
          while (isxdigit((unsigned char)lexer->src[lexer->pos + 1])) {
            char c2 = lexer->src[++lexer->pos];
            val =
                val * 16 +
                (isdigit(c2) ? c2 - '0' : (isupper(c2) ? c2 - 'A' + 10 : c2 - 'a' + 10));
          }
          buffer[bpos++] = (char)val;
        } break;

        // Unicode \uXXXX y \UXXXXXXXX
        case 'u':
        case 'U': {
          int digits = (ch == 'u') ? 4 : 8;
          unsigned int code = 0;
          for (int i = 0; i < digits; i++) {
            char c2 = lexer->src[lexer->pos + 1];
            if (!isxdigit((unsigned char)c2)) break;
            lexer->pos++;
            code =
                code * 16 +
                (isdigit(c2) ? c2 - '0' : (isupper(c2) ? c2 - 'A' + 10 : c2 - 'a' + 10));
          }
          // aquí debes decidir cómo guardar Unicode.
          // Si tu lenguaje es ASCII/UTF-8 simple:
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
          buffer[bpos++] = ch; // copia literal si no se reconoce
          break;
        }
      } else {
        buffer[bpos++] = ch; // carácter normal
      }

      lexer->pos++;
    }

    buffer[bpos] = '\0';
    strcpy(token.tkn_text, buffer);
    token.tkn_type = TKN_STRING;
    lexer->pos++;
  } else if (c == '\'') {
    lexer->pos++;

    char buffer[1024]; // temporal para almacenar string procesada
    size_t bpos = 0;

    while (lexer->src[lexer->pos] != '\'') {
      char ch = lexer->src[lexer->pos];

      if (ch == '\0') {
        lexer->pos++;
        token.tkn_type = TKN_UNDEFINED;
        strcpy(token.tkn_text, "<UNDEF>");
        return token;
      }

      if (ch == '\\') { // secuencia de escape
        lexer->pos++;
        ch = lexer->src[lexer->pos];
        if (ch == '\0') break;

        switch (ch) {
        case 'n': buffer[bpos++] = '\n'; break;
        case 'r': buffer[bpos++] = '\r'; break;
        case 't': buffer[bpos++] = '\t'; break;
        case 'v': buffer[bpos++] = '\v'; break;
        case 'f': buffer[bpos++] = '\f'; break;
        case 'a': buffer[bpos++] = '\a'; break;
        case '\\': buffer[bpos++] = '\\'; break;
        case '\'': buffer[bpos++] = '\''; break;
        case '\"': buffer[bpos++] = '\"'; break;
        case '?': buffer[bpos++] = '?'; break;

        // octales \ooo
        case '0' ... '7': {
          int val = ch - '0';
          int count = 1;
          while (count < 3 && lexer->src[lexer->pos + 1] >= '0' &&
                 lexer->src[lexer->pos + 1] <= '7') {
            val = val * 8 + (lexer->src[++lexer->pos] - '0');
            count++;
          }
          buffer[bpos++] = (char)val;
        } break;

        // hexadecimales \xhh
        case 'x': {
          int val = 0;
          while (isxdigit((unsigned char)lexer->src[lexer->pos + 1])) {
            char c2 = lexer->src[++lexer->pos];
            val =
                val * 16 +
                (isdigit(c2) ? c2 - '0' : (isupper(c2) ? c2 - 'A' + 10 : c2 - 'a' + 10));
          }
          buffer[bpos++] = (char)val;
        } break;

        // Unicode \uXXXX y \UXXXXXXXX
        case 'u':
        case 'U': {
          int digits = (ch == 'u') ? 4 : 8;
          unsigned int code = 0;
          for (int i = 0; i < digits; i++) {
            char c2 = lexer->src[lexer->pos + 1];
            if (!isxdigit((unsigned char)c2)) break;
            lexer->pos++;
            code =
                code * 16 +
                (isdigit(c2) ? c2 - '0' : (isupper(c2) ? c2 - 'A' + 10 : c2 - 'a' + 10));
          }
          // aquí debes decidir cómo guardar Unicode.
          // Si tu lenguaje es ASCII/UTF-8 simple:
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
          buffer[bpos++] = ch; // copia literal si no se reconoce
          break;
        }
      } else {
        buffer[bpos++] = ch; // carácter normal
      }

      lexer->pos++;
    }

    buffer[bpos] = '\0';
    strcpy(token.tkn_text, buffer);
    token.tkn_type = TKN_STRING;
    lexer->pos++;
  } else if (isdigit(c)) {
    if (c == '0') {
      lexer->pos++;
      if (lexer->src[lexer->pos] == 'x' || lexer->src[lexer->pos] == 'X') {
        lexer->pos++;
        size_t start = lexer->pos;
        while (isxdigit(lexer->src[lexer->pos])) {
          lexer->pos++;
        }
        size_t len = lexer->pos - start;
        token.tkn_text[0] = '0';
        token.tkn_text[1] = 'x';
        strncpy(token.tkn_text + 2, lexer->src + start, len);
        token.tkn_text[len + 2] = '\0';
        token.tkn_type = TKN_NUMBER;
      } else if (lexer->src[lexer->pos] == 'b' || lexer->src[lexer->pos] == 'B') {
        lexer->pos++;
        size_t start = lexer->pos;
        while (lexer->src[lexer->pos] == '0' || lexer->src[lexer->pos] == '1') {
          lexer->pos++;
        }
        size_t len = lexer->pos - start;
        token.tkn_text[0] = '0';
        token.tkn_text[1] = 'b';
        strncpy(token.tkn_text + 2, lexer->src + start, len);
        token.tkn_text[len + 2] = '\0';
        token.tkn_type = TKN_NUMBER;
      } else if (lexer->src[lexer->pos] == 'o' || lexer->src[lexer->pos] == 'O') {
        lexer->pos++;
        size_t start = lexer->pos;
        while (lexer->src[lexer->pos] >= '0' && lexer->src[lexer->pos] <= '8') {
          lexer->pos++;
        }
        size_t len = lexer->pos - start;
        token.tkn_text[0] = '0';
        token.tkn_text[1] = 'b';
        strncpy(token.tkn_text + 2, lexer->src + start, len);
        token.tkn_text[len + 2] = '\0';
        token.tkn_type = TKN_NUMBER;
      } else if (lexer->src[lexer->pos] >= '0' && lexer->src[lexer->pos] <= '8') {
        size_t start = lexer->pos;
        while (lexer->src[lexer->pos] >= '0' && lexer->src[lexer->pos] <= '8') {
          lexer->pos++;
        }
        size_t len = lexer->pos - start;
        token.tkn_text[0] = '0';
        strncpy(token.tkn_text + 1, lexer->src + start, len);
        token.tkn_text[len + 1] = '\0';
        token.tkn_type = TKN_NUMBER;
      } else {
        lexer->pos++;
        token.tkn_text[0] = '0';
        token.tkn_text[1] = '\0';
        token.tkn_type = TKN_NUMBER;
      }
    } else {
      size_t start = lexer->pos;
      while (isdigit(lexer->src[lexer->pos])) {
        lexer->pos++;
      }
      size_t len = lexer->pos - start;
      strncpy(token.tkn_text, lexer->src + start, len);
      token.tkn_text[len] = '\0';
      token.tkn_type = TKN_NUMBER;
    }
  } else if (c == '{') {
    lexer->pos++;
    token.tkn_type = TKN_LBRACE;
    token.tkn_text[0] = '{';
    token.tkn_text[1] = '\0';
  } else if (c == '}') {
    lexer->pos++;
    token.tkn_type = TKN_RBRACE;
    token.tkn_text[0] = '}';
    token.tkn_text[1] = '\0';
  } else if (c == '=') {
    lexer->pos++;
    if (lexer->src[lexer->pos] == '>') {
      lexer->pos++;
      token.tkn_type = TKN_BLOCK;
      token.tkn_text[0] = '=';
      token.tkn_text[1] = '>';
      token.tkn_text[2] = '\0';
    } else {
      token.tkn_type = TKN_ASSIGNMENT;
      token.tkn_text[0] = '=';
      token.tkn_text[1] = '\0';
    }
  } else if (lexer->src[lexer->pos] == '(') {
    lexer->pos++;
    token.tkn_type = TKN_LPARENT;
    token.tkn_text[0] = '(';
    token.tkn_text[1] = '\0';
  } else if (lexer->src[lexer->pos] == ')') {
    lexer->pos++;
    token.tkn_type = TKN_RPARENT;
    token.tkn_text[0] = ')';
    token.tkn_text[1] = '\0';
  } else if (lexer->src[lexer->pos] == ',') {
    lexer->pos++;
    token.tkn_type = TKN_COMMA;
    token.tkn_text[0] = ',';
    token.tkn_text[1] = '\0';
  } else {
    token.tkn_text[0] = lexer->src[lexer->pos];
    token.tkn_text[1] = '\0';
    lexer->pos++;
    token.tkn_type = TKN_UNDEFINED;
  }
  return token;
}
