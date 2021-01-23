#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stdlib.h>


enum token_type
{
    LEFT_PAREN = 0, RIGHT_PAREN = 1, LEFT_BRACE = 2, RIGHT_BRACE = 3, 
    MINUS = 4, PLUS = 5, SEMICOLON = 6, COMMA = 7, ASTERISK = 8, SLASH = 9,

    BANG = 10, EQUAL = 11, BANG_EQUAL = 12, EQUAL_EQUAL = 13, GREATER = 14, GREATER_EQUAL = 15,
    LESS = 16, LESS_EQUAL = 17,

    ARROW = 18,

    INTEGER = 19, LONG_INTEGER = 20, IDENTIFIER = 21,

    // reserved keywords

    INT = 22, LONG = 23, VOID = 24, OR = 25, AND = 26, 
    IF = 27, ELSE = 28, OUTPUT = 29, RETURN = 30, WHILE = 31, FUNCTION = 32, FOR = 33, MAIN = 34,

    EOF_F = 35
};

struct token
{
    enum token_type type;
    union {
        char* string_v;        // identifiers store string_v on heap -> therefore, free.
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
    char key[7];
    enum token_type tkn_type;
};

int compute_hash (char* k);
struct kw_item* construct_map();
void insert_entry (char* k, enum token_type tag, struct kw_item* map);
enum token_type get_tkn_type (char* k, struct kw_item* map);

struct token_vector* construct_vector (); // free token_vector and vec
void add_token (struct token_vector* tkn_vec, enum token_type tag, void* value); 

#endif
