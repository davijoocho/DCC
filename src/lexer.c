
#include "lexer.h"

struct token_vector* scan (struct src_string* src)
{
    struct token_vector* tkn_vec = construct_vector();

    while (!is_at_end()) {
        src->begin = src->forward;
        read_token(src, tkn_vec);
    }

    // add EOF_F to token_vector;
}

void read_token (struct src_string* src, struct token_vector* tkn_vec)
{}

bool is_at_end (struct src_string* src) { return src->forward >= src->length; }

char advance (struct src_string* src) { return src->str[cur++]; }

bool match (struct src_string* src, char expected)
{
    if (is_at_end()) return false; // this should be an error?
    if (src->str[cur] != expected) return false;
    cur++;
    return true;
}

