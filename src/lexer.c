#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
/*
   NOT CHECKING FOR ERRORS, DO WHEN YOU'RE DONE IMPLEMENTING REST OF COMPILER
   */

int compute_perfect_hash(char* s, int len) {
    int hash = 0;
    for (int i = 0; i < len; i++) {
        hash = hash + s[i];
    }

    switch (s[0]) {
        case 's':
            if (len == 6) {
                if (s[3] == 'u') {  // struct
                    hash += 19;
                } else if (s[3] == 'i') {  // string
                    hash += 10;
                }
            }
            break;
        case 'E':  // Empty
            hash += 3;
            break;
        case 'a':
            if (len > 2) {
                if (s[1] == 'l') {   // allocate
                    hash += 6;
                } else if (s[1] == 'n') {  // and
                    hash += 9;
                }
            }
            break;

        case 'f':
            if (len > 1) {
                if (s[1] == 'n') {   // fn
                    hash += 12;
                } else if (s[1] == '6') {   // f64
                    hash += 14;
                } else if (s[1] == '3') {  // f32
                    hash += 20;
                } else if (s[1] == 'a') {  // false
                    hash += 10;
                }
            }
            break;

        case 'e':
            if (len == 4) {
                if (s[2] == 's') {  // else
                    hash += 16;
                } else if (s[2] == 'i') {   // elif
                    hash += 2;
                }
            }
            break;
        case 'o':
            if (len == 2 && s[1] == 'r') {  // or
                hash += 2;
            }
            break;
        case 'i':
            if (len == 2) {
                if (s[1] == 's') {  // is
                    hash += 8;
                } else if (s[1] == 'f') {  // if
                    hash += 1;
                }
            } else if (len > 2) {
                if (s[1] == 's') {  // isnt
                    hash += 23;
                } else if (s[1] == '6') {  // i64
                    hash += 20;
                }
            }
            break;
        case 'b': hash += 21; break;  // bool
        case 't': hash += 2; break;  // true
        case 'r': hash += 19; break;  // return
        case 'c': hash += 9; break;   // c8
        case 'p': hash += 17; break;  // proc
        case 'w': hash += 13; break;  // while
        case 'n': hash += 22; break;  // not
        default:
            break;
    }
    return hash % N_KEYWORDS;
}

int get_token_type(char* s, int len, char** kw_map) {
    int hash = compute_perfect_hash(s, len);
    char* kw = kw_map[hash];

    if (strlen(kw) != len) {
        return IDENTIFIER;
    }

    int res = strncmp(kw, s, len);
    if (res != 0) {
        return IDENTIFIER;
    }

    enum token_type type[] = {STRUCT, STRING, EMPTY, ALLOCATE, AND, FALSE, F64, F32,
                                FUNCTION, ELSE, ELIF, OR, IS, ISNT, I32, I64, IF, BOOL, TRUE, RETURN,
                                C8, PROCEDURE, WHILE, NOT};

    return type[hash];
}

void add_token(struct tokens* toks, enum token_type type, struct scan_md* s_md, int lbp, long len) {
    if (TOKEN_OVERFLOW(toks)) {
        GROW_TOKEN_SPACE(toks);
    }

    if (type == IDENTIFIER && PREV_TOKEN(toks)->type == IDENTIFIER) {
        PREV_TOKEN(toks)->type = STRUCT_ID;
    }

    toks->vec[toks->n] = malloc(sizeof(struct token));
    struct token* tok = toks->vec[toks->n++]; 

    INIT_TOKEN(tok, type, lbp, s_md->line);
    ASSIGN_LITERAL_VALUE(tok, type, s_md)
}

void read_token(struct scan_md* s_md, struct tokens* toks, long len, char** kw_map) {
    char c = s_md->src[s_md->end++];

    switch (c) {
        case '+': add_token(toks, PLUS, s_md, 80, len); break;
        case '*': add_token(toks, STAR, s_md, 90, len); break;
        case '~': add_token(toks, BIT_NOT, s_md, 0, len); break;
        case '|': add_token(toks, BIT_OR, s_md, 30, len); break;
        case '&': add_token(toks, BIT_AND, s_md, 40, len); break;
        case ',': add_token(toks, COMMA, s_md, 0, len); break;
        case ':': add_token(toks, COLON, s_md, 0, len); break;
        case '%': add_token(toks, MODULO, s_md, 90, len); break;
        case '(': add_token(toks, LEFT_PAREN, s_md, 0, len); break;
        case ')': add_token(toks, RIGHT_PAREN, s_md, 0, len); break;
        case '[': add_token(toks, LEFT_BRACK, s_md, 100, len); break;
        case ']': add_token(toks, RIGHT_BRACK, s_md, 0, len); break;
        case '=': add_token(toks, ASSIGN, s_md, 0, len); break;
        case '\'': add_token(toks, CHARACTER, s_md, 0, len); break;

        case '\n':
              add_token(toks, NEW_LINE, s_md, 0, len); 
              s_md->line++;
              break;

        case '-':
              if (s_md->src[s_md->end] == '>') {
                  s_md->end++;
                  add_token(toks, ARROW, s_md, 100, len);
              } else {
                  add_token(toks, MINUS, s_md, 80, len);
              }
              break;

        case '/':
              c = s_md->src[s_md->end];
              if (c == '/') {
                  do {
                      c = s_md->src[++s_md->end];
                  } while (c != '\n');
              } else {
                  if (c == '*') {
                      int8_t stop = 0;
                      while (!stop) {
                          if (s_md->src[++s_md->end] == '*' && 
                                  s_md->src[s_md->end + 1] == '/') {
                              s_md->end += 2;
                              stop = 1;
                          }
                      }
                  } else {
                      add_token(toks, DIVIDE, s_md, 90, len);
                  }
              }
              break;

        case '<':
              if (s_md->src[s_md->end] == '=') {
                  s_md->end++;
                  add_token(toks, LTEQ, s_md, 60, len);
              } else if (s_md->src[s_md->end] == '<') {
                  s_md->end++;
                  add_token(toks, SHIFT_L, s_md, 70, len);
              } else {
                  add_token(toks, LT, s_md, 60, len);
              }
              break;

        case '>':
              if (s_md->src[s_md->end] == '=') {
                  s_md->end++;
                  add_token(toks, GTEQ, s_md, 60, len);
              } else if (s_md->src[s_md->end] == '>') {
                  s_md->end++;
                  add_token(toks, SHIFT_R, s_md, 70, len);
              } else {
                  add_token(toks, GT, s_md, 60, len);
              }
              break;

        case ' ': 
              while (s_md->src[s_md->end] == ' ')
                  s_md->end++;

              if ((toks->vec[toks->n - 1])->type == NEW_LINE) {
                  add_token(toks, SPACES, s_md, 0, len);
              } 
              break;
        case '"':
              while (s_md->src[s_md->end++] != '\"');
              add_token(toks, STRING_LITERAL, s_md, 0, s_md->end - s_md->start - 2);
              break;

        default:
            if (IS_NUMERIC(c)) {
                c = s_md->src[s_md->end];
                enum token_type type = INTEGER;

                while (IS_NUMERIC(c)) {
                    c = s_md->src[++s_md->end];
                    if (c == '.') {
                        type = DOUBLE;
                        c = s_md->src[++s_md->end];
                    }
                }

                if (type == DOUBLE && c == 'F') {
                    type = FLOAT;
                    s_md->end++;
                } else {
                    if (c == 'L') {
                        type = LONG;
                        s_md->end++;
                    }
                }

                add_token(toks, type, s_md, 0, len);
                break;
            }

            if (IS_ALPHA(c)) {
                c = s_md->src[s_md->end];
                while (IS_ALPHA(c) || IS_NUMERIC(c))
                    c = s_md->src[++s_md->end];

                enum token_type type = get_token_type(s_md->src + s_md->start, s_md->end - s_md->start, kw_map);
                int bp = 0;
                if (type == AND) {
                    bp = 20;
                } else if (type == OR) {
                    bp = 10;
                } else if (type == IS || type == ISNT) {
                    bp = 50;
                }
                add_token(toks, type, s_md, bp, s_md->end - s_md->start); 
            }
            break;
    }
}

struct tokens* scan(char* src, long len) {
    struct scan_md s_md = {src, 0, 0, 0};
    struct tokens* toks = malloc(sizeof(struct tokens));
    char* kw_map[] = {"struct", "string", "Empty", "allocate", "and", "false",
                        "f64", "f32", "fn", "else", "elif", "or", "is", "isnt",
                        "i32", "i64", "if", "bool", "true", "return", "c8", "proc",
                        "while", "not"};

    toks->vec = malloc(sizeof(struct token*) * 512);
    toks->n = 0; 
    toks->i = 0;
    toks->size = 512;

    while (s_md.start < len) {
        read_token(&s_md, toks, len, kw_map);
        s_md.start = s_md.end;
    }

    free(src);
    return toks;
}
