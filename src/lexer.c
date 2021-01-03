
#include "token.h"
#include "lexer.h"


bool is_at_end (struct src_string* src) { return src->forward >= src->length; }

struct token_vector* scan (struct src_string* src)
{
    
    struct token_vector* = construct_vector();

    while (!is_at_end()) {
        src->begin = src->forward;
        read_token(src);
    }

}
