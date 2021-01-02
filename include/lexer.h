#ifndef LEXER_H
#define LEXER_H

#include "token.h"


struct src_string
{
    int beg;
    int cur;
    uint8_t* str;
}



struct token_vector* scan (uint8_t* src_str);   // after done scanning free src in src_string
struct src_string* construct_src (struct src_string* src, uint8_t* src_str);

bool is_at_end (struct src_string* src);
uint8_t peek (struct src_string* src);
uint8_t peek_next (struct src_string* src);
uint8_t advance (struct src_string* src);

void tokenize (struct src_string* src, struct token_vector* vec);

bool is_digit (uint8_t c);
bool is_alpha (uint8_t c);

void to_integer (struct src_string* src, struct token_vector* vec);
void to_identifier (struct src_string* src, struct token_vector* vec);

#endif
