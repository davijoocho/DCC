#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "token.h"
#include "lexer.h"

struct token_vector* scan (struct src_string* src)
{
    struct kw_item* kw_hmap = construct_map();
    struct token_vector* tkn_vec = construct_vector();

    while (!is_at_end(src)) {
        src->begin = src->forward;
        read_token(src, tkn_vec, kw_hmap);
    }

    add_token(tkn_vec, EOF_F, NULL);
    free(kw_hmap);
    free(src->str);
    return tkn_vec;
}

void read_token (struct src_string* src, struct token_vector* tkn_vec, struct kw_item* kw_hmap)
{
    char c = advance(src);

    switch (c) {

        case '(': add_token(tkn_vec, LEFT_PAREN, NULL); break;
        case ')': add_token(tkn_vec, RIGHT_PAREN, NULL); break;
        case '{': add_token(tkn_vec, LEFT_BRACE, NULL); break;
        case '}': add_token(tkn_vec, RIGHT_BRACE, NULL); break;
        case '-': add_token(tkn_vec, MINUS, NULL); break;
        case '+': add_token(tkn_vec, PLUS, NULL); break;
        case ';': add_token(tkn_vec, SEMICOLON, NULL); break;
        case '*': add_token(tkn_vec, ASTERISK, NULL); break;
        case ',': add_token(tkn_vec, COMMA, NULL); break;
        case '/':
            if (match(src, '/')) {
                while (peek(src) != '\n' && !is_at_end(src)) advance(src);
            } else {
                add_token(tkn_vec, SLASH, NULL);
            }
            break;

        case '!': add_token(tkn_vec, match(src, '=') ? BANG_EQUAL : BANG, NULL); break;
        case '>': add_token(tkn_vec, match(src, '=') ? GREATER_EQUAL : GREATER, NULL); break;
        case '<': add_token(tkn_vec, match(src, '=') ? LESS_EQUAL : LESS, NULL); break;
        case '=': 
            if (match(src, '=')) {
                add_token(tkn_vec, EQUAL_EQUAL, NULL);
            } else if (match(src, '>')) {
                add_token(tkn_vec, ARROW, NULL);
            } else {
                add_token(tkn_vec, EQUAL, NULL);
            }
            break;
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            break;

        default:
            if (is_digit(c)) {
                read_number(src, tkn_vec);
            } else if (is_alpha(c)) {
                read_identifier(src, tkn_vec, kw_hmap);
            }
            break;
    }

}

bool is_at_end (struct src_string* src) { return src->forward >= src->length; }
char advance (struct src_string* src) { return src->str[src->forward++]; }

bool is_digit (char c) { return '0' <= c && c <= '9'; }
bool is_alpha (char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_'; }

char peek (struct src_string* src) { return (is_at_end(src)) ? '\0' : src->str[src->forward];}
char peek_next (struct src_string* src) { return (src->forward + 1 >= src->length) ? '\0' : src->str[src->forward + 1]; }

bool match (struct src_string* src, char expected)
{
    if (src->str[src->forward] != expected) return false;
    src->forward++;
    return true;
}


//refactor
void read_number (struct src_string* src, struct token_vector* vec) 
{
    while (is_digit(peek(src))) advance(src);

    int n_bytes = src->forward - src->begin;

    char str_n[n_bytes + 1];
    char cmp[n_bytes + 1];
    char* pos = src->str + src->begin;

    long long_v;

    if (n_bytes > 10) {
        strncpy(str_n, pos, n_bytes);
        str_n[n_bytes] = '\0';
        long_v = atol(str_n);
        add_token(vec, LONG_INTEGER, &long_v);
        return;
    }

    strncpy(str_n, pos, n_bytes);
    str_n[n_bytes] = '\0';

    int int_v = atoi(str_n);
    sprintf(cmp, "%d", int_v);

    if (!strcmp(str_n, cmp)) {
        add_token(vec, INTEGER, &int_v);
    } else {
        long_v = atol(str_n);
        add_token(vec, LONG_INTEGER, &long_v); 
    }

    return;
}

//refactor
void read_identifier (struct src_string* src, struct token_vector* vec, struct kw_item* kw_hmap)
{
    while ( is_digit(peek(src)) || is_alpha(peek(src)) ) advance(src);

    int n_bytes = src->forward - src->begin;
    char* lexeme = malloc( n_bytes + 1 );
    char* pos = src->str + src->begin;
    strncpy(lexeme, pos, n_bytes); 
    lexeme[n_bytes] = '\0';

    enum token_type type = get_tkn_type(lexeme, kw_hmap);

    if (type == -1) {
        add_token(vec, IDENTIFIER, lexeme);
    } else {
        free(lexeme);
        add_token(vec, type, NULL);
    }

    return;
}
