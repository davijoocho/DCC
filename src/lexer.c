#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lexer.h"




uint32_t compute_hash(char* str, int len) {
    uint32_t hash = 0;
    for (int i = 0; i < len; i++) {
        hash += str[i];
    }
    return hash % KEYWORD_HASHTAB_SIZE;
}

void construct_keyword_hashtab(struct keyword_entry* keyword_hashtab) {
    memset(keyword_hashtab, 0, sizeof(struct keyword_entry) * KEYWORD_HASHTAB_SIZE);
    char* keywords[] = {"or", "and", "is", "isnt", "not", "Empty", "c8", "i32", "i64", 
        "f32", "f64", "string", "fn", "proc", "struct", "main", "if", "elif", "else", 
        "while", "ret", "free", "open", "write", "read", "close", "memcpy", "print", "malloc", "_FILE", "realloc"};
    enum token_type types[] = {LOGICAL_OR, LOGICAL_AND, IS, ISNT, LOGICAL_NOT, EMPTY, C8, I32, I64,
        F32, F64, STRING, FUNCTION, PROCEDURE, STRUCT, MAIN, IF, ELIF, ELSE,
        WHILE, RETURN, FREE, OPEN, WRITE, READ, CLOSE, MEMCPY, PRINT, MALLOC, _FILE, REALLOC};

    for (int i = 0; i < N_KEYWORDS; i++) {
        uint32_t idx = compute_hash(keywords[i], strlen(keywords[i]));

        while (keyword_hashtab[idx].occupied)
            if (++idx == KEYWORD_HASHTAB_SIZE)
                idx = 0;

        struct keyword_entry* entry = &keyword_hashtab[idx];
        entry->occupied = 1;
        entry->keyword = keywords[i];
        entry->type = types[i];
    }
};


void add_token(enum token_type type, int lbp, struct scanner_info* scan_info, struct tokens* token_lst) {
    if (token_lst->n_tokens == token_lst->capacity) {
        token_lst->tokens = realloc(token_lst->tokens, 8 * token_lst->capacity * 2);
        token_lst->capacity *= 2;
    }

    struct token* token = malloc(sizeof(struct token));
    token->type = type;
    token->lbp = lbp;
    token->line = scan_info->line;

    switch (type) {
        case STRING_LITERAL:
            token->string = malloc(STRING_LEN(scan_info) + 1);
            strncpy(token->string, scan_info->source + scan_info->start + 1, STRING_LEN(scan_info));
            token->string[STRING_LEN(scan_info)] = '\0';
            break;
        case INDENT: token->i32 = scan_info->end - scan_info->start; break;
        case CHARACTER:
              if (scan_info->source[scan_info->end] == '\\') {
                  char c = scan_info->source[scan_info->end++];
                  if (c == '0') {
                      token->c8 = '\0';
                  } else if (c == '\'') {
                      token->c8 = '\'';
                  }
              } else {
                  token->c8 = scan_info->source[scan_info->end]; 
              }

              scan_info->end += 2;
              break;
        case INTEGER: token->i32 = atoi(scan_info->source + scan_info->start); break;
        case LONG: 
              token->i64 = atol(scan_info->source + scan_info->start);
              scan_info->end++;
              break;
        case FLOAT: 
              token->f32 = strtof(scan_info->source + scan_info->start, NULL); 
              scan_info->end++;
              break;
        case DOUBLE: 
              token->f64 = strtod(scan_info->source + scan_info->start, NULL);
              scan_info->end++;
              break;

        case OPEN:
        case CLOSE:
        case READ:
        case WRITE:
        case MEMCPY:
        case MAIN:
        case PRINT:
        case MALLOC:
        case REALLOC:
        case _FILE:
        case STRUCT_ID:
        case IDENTIFIER:
            token->string = malloc(scan_info->end - scan_info->start + 1);
            strncpy(token->string, scan_info->source + scan_info->start, scan_info->end - scan_info->start);
            token->string[scan_info->end - scan_info->start] = '\0';
            break;
        default:
            break;
    }

    // DEBUGGING PURPOSES
    token->lexeme = malloc(scan_info->end - scan_info->start + 1);
    strncpy(token->lexeme, scan_info->source + scan_info->start, scan_info->end - scan_info->start);
    token->lexeme[scan_info->end - scan_info->start] = '\0';

    token_lst->tokens[token_lst->n_tokens++] = token;
}

void scan(struct scanner_info* scan_info, struct keyword_entry* keyword_hashtab, struct tokens* token_lst) {

    switch (scan_info->source[scan_info->end++]) {
        case '|': add_token(BIT_OR, 30, scan_info, token_lst); break;
        case '&': add_token(AND, 40, scan_info, token_lst); break;
        case '<': 
            if (scan_info->source[scan_info->end] == '<') {
                scan_info->end++;
                add_token(BIT_SHIFTL, 70, scan_info, token_lst);
            } else if (scan_info->source[scan_info->end] == '=') {
                scan_info->end++;
                add_token(LTEQ, 60, scan_info, token_lst);
            } else {
                add_token(LT, 60, scan_info, token_lst);
            }
            break;
        case '>':
            if (scan_info->source[scan_info->end] == '>') {
                scan_info->end++;
                add_token(BIT_SHIFTR, 70, scan_info, token_lst);
            } else if (scan_info->source[scan_info->end] == '=') {
                scan_info->end++;
                add_token(GTEQ, 60, scan_info, token_lst);
            } else {
                add_token(GT, 60, scan_info, token_lst); 
            }
            break;
        case '+': add_token(PLUS, 80, scan_info, token_lst); break;
        case '-':
            if (scan_info->source[scan_info->end] == '>') {
                scan_info->end++;
                add_token(ARROW, 100, scan_info, token_lst); 
            } else {
                add_token(MINUS, 80, scan_info, token_lst);
            }
            break;

        case '/': {
            char c = scan_info->source[scan_info->end];
            if (c == '/') {
                do {
                    c = scan_info->source[++scan_info->end];
                } while (c != '\n');
            } else if (c == '*') {
                int stop = 0;
                while (!stop) {
                    if (scan_info->source[++scan_info->end] == '*'
                            && scan_info->source[scan_info->end + 1] == '/') {
                        scan_info->end += 2;
                        stop = 1;
                    }
                }
            } else {
                add_token(DIVIDE, 90, scan_info, token_lst); 
            }
        }
            break;
        case '*': add_token(STAR, 90, scan_info, token_lst); break;
        case '%': add_token(MODULO, 90, scan_info, token_lst); break;
        case '[': add_token(LEFT_BRACK, 100, scan_info, token_lst); break;
        case ']': add_token(RIGHT_BRACK, 0, scan_info, token_lst); break;
        case '.': add_token(DOT, 100, scan_info, token_lst); break;
        case '(': add_token(LEFT_PAREN, 0, scan_info, token_lst); break;
        case ')': add_token(RIGHT_PAREN, 0, scan_info, token_lst); break;
        case '{': add_token(LEFT_BRACE, 0, scan_info, token_lst); break;
        case '}': add_token(RIGHT_BRACE, 0, scan_info, token_lst); break;
        case ',': add_token(COMMA, 0, scan_info, token_lst); break;
        case ':': add_token(COLON, 0, scan_info, token_lst); break;
        case '=': add_token(ASSIGN, 0, scan_info, token_lst); break;
        case '\'': 
              add_token(CHARACTER, 0, scan_info, token_lst); 
              break;
        case '\n': 
              if (token_lst->n_tokens != 0 && token_lst->tokens[token_lst->n_tokens - 1]->type != NEW_LINE) {
                  add_token(NEW_LINE, 0, scan_info, token_lst);
              }
              scan_info->line++; 
              break;
        case '"':
              while (scan_info->source[scan_info->end++] != '"');
              add_token(STRING_LITERAL, 0, scan_info, token_lst);
              break;
        case ' ':
              while (scan_info->source[scan_info->end] == ' ') {
                  scan_info->end++;
              }

              if (scan_info->source[scan_info->end] == '\n') {
                  scan_info->end++;
                  scan_info->line++;
                  break;
              }

              if (token_lst->tokens[token_lst->n_tokens - 1]->type == NEW_LINE) {
                  add_token(INDENT, 0, scan_info, token_lst);
              }
              break;
        default: {
              char c = scan_info->source[scan_info->end - 1];

              if (IS_NUMERIC(c)) {
                  char nxt = scan_info->source[scan_info->end];
                  int n_deci = 0;
                  while (IS_NUMERIC(nxt) || nxt == '.') {
                      nxt = scan_info->source[++scan_info->end];
                  }

                  if (nxt == 'L') {
                      add_token(LONG, 0, scan_info, token_lst);
                  } else if (nxt == 'F') {
                      add_token(FLOAT, 0, scan_info, token_lst);
                  } else if (nxt == 'D') {
                      add_token(DOUBLE, 0, scan_info, token_lst);
                  } else {
                      add_token(INTEGER, 0, scan_info, token_lst);
                  }

              } else if (c == '_') {
                  char nxt = scan_info->source[scan_info->end];
                  while (IS_ALPHA(nxt) || IS_NUMERIC(nxt)) {
                      nxt = scan_info->source[++scan_info->end];
                  }

                  add_token(STRUCT_ID, 0, scan_info, token_lst);

              } else if (IS_ALPHA(c)) {
                  char nxt = scan_info->source[scan_info->end];
                  while (IS_ALPHA(nxt) || IS_NUMERIC(nxt)) {
                      nxt = scan_info->source[++scan_info->end];
                  }

                  uint32_t idx = compute_hash(scan_info->source + scan_info->start, scan_info->end - scan_info->start);

                  while (keyword_hashtab[idx].occupied) {
                      struct keyword_entry* entry = &keyword_hashtab[idx++];
                      if (!strncmp(entry->keyword, scan_info->source + scan_info->start, scan_info->end - scan_info->start)) {
                          int lbp = (entry->type == LOGICAL_OR) ? 10 : (entry->type == LOGICAL_AND) ? 20 : 
                              (entry->type == IS || entry->type == ISNT) ? 50 : 0;
                          add_token(entry->type, lbp, scan_info, token_lst);
                          return;
                      }
                      if (idx == KEYWORD_HASHTAB_SIZE)
                          idx = 0;
                  }

                  add_token(IDENTIFIER, 0, scan_info, token_lst);
              }
        }
    }
}

struct tokens* lexical_analysis(char* source, long len) {
    struct scanner_info scan_info = {.source = source, .start = 0, .end = 0, .line = 0};
    struct tokens* token_lst = malloc(sizeof(struct tokens));
    struct keyword_entry keyword_hashtab[KEYWORD_HASHTAB_SIZE];

    construct_keyword_hashtab(keyword_hashtab);
    token_lst->tokens = malloc(sizeof(struct token*) * 512);
    token_lst->n_tokens = 0;
    token_lst->capacity = 512;
    token_lst->idx = 0;

    while (scan_info.start < len) {
        scan(&scan_info, keyword_hashtab, token_lst);
        scan_info.start = scan_info.end;
    }
   
    add_token(EOFF, 0, &scan_info, token_lst);
    free(source);

    return token_lst;
}

