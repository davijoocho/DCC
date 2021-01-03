#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include "token.h"

struct src_string
{
    int begin;
    int forward;
    int length;
    char* str;
};

// token hash_table

struct token_vector* scan (struct src_string* src);   // after done scanning free src in src_string

void read_token (struct src_string* src, struct token_vector* tkn_vec);

bool is_at_end (struct src_string* src); //

char peek (struct src_string* src);
char peek_next (struct src_string* src);

bool match (struct src_string* src, char expected); //
char advance (struct src_string* src); //


bool is_digit (char c);
bool is_alpha (char c);

void to_integer (struct src_string* src, struct token_vector* vec);
void to_identifier (struct src_string* src, struct token_vector* vec);

#endif
