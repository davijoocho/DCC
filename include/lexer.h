#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include "token.h"

struct src_string
{
    int begin;
    int forward;
    int length;
    char* str;
};

struct token_vector* scan (struct src_string* src);  //

void read_token (struct src_string* src, struct token_vector* tkn_vec, struct kw_item* kw_hmap); // 

bool is_at_end (struct src_string* src); //

char peek (struct src_string* src); //
char peek_next (struct src_string* src); //

bool match (struct src_string* src, char expected); //
char advance (struct src_string* src); //

bool is_digit (char c); //
bool is_alpha (char c); //

void read_number (struct src_string* src, struct token_vector* vec);
void read_identifier (struct src_string* src, struct token_vector* vec, struct kw_item* kw_map);

#endif
