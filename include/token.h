#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>

enum token_type
{
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, 
    MINUS, PLUS, SEMICOLON, ASTERISK, COMMA, SLASH,


    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    ARROW,

    INTEGER, IDENTIFIER,

    // reserved keywords

    INT = 21, AND = 22, ELSE = 23, INSTRUCTN = 24, FOR = 25, 
    IF = 26, OR = 27, OUTPUT = 28, RETURN = 29, WHILE = 30, LONG = 31,

    EOF_F
};

struct token
{
    enum token_type type;
    union {
        char* string_v;
        int int_v;
        long long_v;
    };
};

struct token_vector
{
    int pos;
    int n_items;
    int max_length;
    struct token* vec;
};

struct kw_item
{
    bool is_reserved;
    char key[10];
    enum token_type tkn_type;
};

int compute_hash (char* k);
struct kw_item* construct_map();
void add_entry (char* k, enum token_type tag, struct kw_item* map);
enum token_type get_tkn_type (char* k, struct kw_item* map);

struct token_vector* construct_vector (); // malloc and ret pointer - free token_vector and vec
void add_token (struct token_vector* tkn_vec, enum token_type tag, void* value); 

#endif
