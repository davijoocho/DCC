
#include <stddef.h>
#include "token.h"
#include "lexer.h"

struct token_vector* scan (struct src_string* src)
{
    struct kw_item* kw_hmap[10] = construct_map();
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
        //here
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
        // here
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
    if (src->str[forward] != expected) return false;
    forward++;
    return true;
}

void read_number (struct src_string* src, struct token_vector* vec) 
{
    while (is_digit(peek(src))) advance();
    // check if number contains a decimal for floating point numbers. 
}

void read_identifier (struct src_string* src, struct token_vector* vec, struct kw_item* kw_hmap);
