
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "token.h"



struct kw_item* construct_map()
{
    struct kw_item* map = calloc(10, sizeof(struct kw_item));
    char* reserved_kw[11] = {"int", "and", "else", "instructn", "for", "if", "or", "output", "return", "while", "long"};
    
    for (int e = 21, int i = 0; i < 11; e++, i++) {
        add_entry(reserved_kw[i], e, map);
    }

    return map;
}


int compute_hash (char* k)
{
    int h = 0;
    int i = 0;

    while (k[i] != '\0') {
        h = (31 * h + k[i]) % 11;
        i++;
    }
    return h;
}


void add_entry(char* k, enum token_type tag, struct kw_item* map)
{
    int i = compute_hash(k);
    struct kw_item* kw_i = map[i];

    while (kw_i->is_reserved) {
        if (++i == 11) i = 0;
        kw_i = map[i];
    }

    kw_i->is_reserved = true;
    strcpy(kw_i->key, k);
    kw_i->tkn_type = tag;
}

enum token_type get_tkn_type (char* k, struct kw_item* map)
{
    int i = compute_hash(k);
    int stop = i;
    struct kw_item* kw_i = map[i];

    do {
        if (!strcmp(kw_i->key, k)) return kw_i->tkn_type;
        if (++i == 11) i = 0;
        kw_i = map[i];
    } while ( i != stop && kw_i->is_reserved );

    return -1;
}

struct token_vector* construct_vector()
{
    struct token_vector* tkn_vec = malloc( sizeof(struct token_vector) );
    tkn_vec->pos = 0;
    tkn_vec->length = 0;
    tkn_vec->max_length = 50;
    tkn_vec->vec = malloc( sizeof(struct token) * tkn_vec->max_length );
    return tkn_vec;
}

// change when more types are added
void add_token (struct token_vector* tkn_vec, enum token_type tag, void* value)
{
    if (tkn_vec->length == tkn_vec->max_length) tkn_vec->vec = realloc( tkn_vec->vec, tkn_vec->max_length *= 2 );

    struct token* tkn = &tkn_vec->vec[tkn_vec->length++];
    tkn->type = tag;

    switch (tag) {
        case INT: tkn->int_v = *(int*)value; break;
        case LONG: tkn->long_v = *(long*)value; break;
        default: tkn->string_v = value; break;
    }

    return;
}
