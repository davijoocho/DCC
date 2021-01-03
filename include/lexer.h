#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include "token.h"

struct src_string
{
    int begin;
    int forward;
    int length;
    uint8_t* str;
};

struct token_vector* scan (struct src_string* src);   // after done scanning free src in src_string

bool is_at_end (struct src_string* src);
uint8_t peek (struct src_string* src);
uint8_t peek_next (struct src_string* src);
uint8_t advance (struct src_string* src);

void read_token (struct src_string* src, struct token_vector* vec);

bool is_digit (uint8_t c);
bool is_alpha (uint8_t c);

void to_integer (struct src_string* src, struct token_vector* vec);
void to_identifier (struct src_string* src, struct token_vector* vec);

#endif
