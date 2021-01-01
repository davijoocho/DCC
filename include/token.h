#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>

enum token_type
{
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    MINUS, PLUS, SEMICOLON, SLASH, ASTERISK,


    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    INT, IDENTIFIER,

    AND, ELSE, INSTRUCTN, FOR, IF, OR, OUTPUT, RETURN, WHILE, 

    EOF_F
};

struct token
{
    enum token_type tag;
    union
    {
        char* lexeme;
        int int_v;
    };
};

struct token_vector
{
    int n_items;
    int max_size;
    struct token* vec;
};

struct token_vector* construct_vector (); // malloc and ret pointer
void add_token (struct token_vector* tkn_vec, enum token_type type, void* value); // realloc when n_items = max_size

#endif
