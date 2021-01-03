
#include "token.h"


struct token_vector* construct_vector()
{
    struct token_vector* tkn_vec = malloc( sizeof(struct token_vector) );
    tkn_vec->pos = 0;
    tkn_vec->length = 0;
    tkn_vec->max_length = 50;
    tkn_vec->vec = malloc( sizeof(struct token) * 50 );
    return tkn_vec;
}



// change when more types are added
void add_token (struct token_vector* tkn_vec, enum token_type type, void* value)
{
    if (tkn_vec->length == tkn_vec->max_length) {
        tkn_vec->vec = realloc( tkn_vec->vec, tkn_vec->max_length * 2 );
        tkn_vec->max_length *= 2;
    }

    struct token tkn;
    tkn.tag = type;
    (type == INT) ? tkn.int_v = *(int*)value : tkn.string_v = value;  

    tkn_vec->vec[tkn_vec->length++] = tkn;

    return;
}
