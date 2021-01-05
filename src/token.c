#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "token.h"

#define N_KEYWORDS 13

struct kw_item* construct_map()
{
    struct kw_item* map = calloc(N_KEYWORDS, sizeof(struct kw_item));
    char* reserved_kw[N_KEYWORDS] = {"int", "and", "else", "instructn", "for", "if", "or", "output", "return", "while", "long", "void", "main"};
    
    int e, i;
    for (e = 22, i = 0; i < N_KEYWORDS; e++, i++) {
        add_entry(reserved_kw[i], e, map);
    }

    return map;
}


int compute_hash (char* k)
{
    int h = 0;
    int i = 0;

    while (k[i] != '\0') {
        h = (31 * h + k[i]) % N_KEYWORDS;
        i++;
    }
    return h;
}


void add_entry(char* k, enum token_type tag, struct kw_item* map)
{
    int i = compute_hash(k);
    struct kw_item* kw_i = &map[i];

    while (kw_i->is_reserved) {
        if (++i == N_KEYWORDS) i = 0;
        kw_i = &map[i];
    }

    kw_i->is_reserved = true;
    strcpy(kw_i->key, k);
    kw_i->tkn_type = tag;

    return;
}

enum token_type get_tkn_type (char* k, struct kw_item* map)
{
    int i = compute_hash(k);
    int stop = i;
    struct kw_item* kw_i = &map[i];

    do {
        if (!strcmp(kw_i->key, k)) return kw_i->tkn_type;
        if (++i == N_KEYWORDS) i = 0;
        kw_i = &map[i];
    } while ( i != stop && kw_i->is_reserved );

    return -1;
}

struct token_vector* construct_vector()
{
    struct token_vector* tkn_vec = malloc( sizeof(struct token_vector) );
    tkn_vec->pos = 0;
    tkn_vec->n_items = 0;
    tkn_vec->max_length = 128;
    tkn_vec->vec = malloc( sizeof(struct token) * tkn_vec->max_length );
    return tkn_vec;
}

// change when more types are added
void add_token (struct token_vector* tkn_vec, enum token_type tag, void* value)
{
    if (tkn_vec->n_items == tkn_vec->max_length) tkn_vec->vec = realloc( tkn_vec->vec, sizeof(struct token) * (tkn_vec->max_length *= 2) );

    struct token* tkn = &tkn_vec->vec[tkn_vec->n_items++];
    tkn->type = tag;

    switch (tag) {
        case INTEGER: tkn->int_v = *(int*)value; break;
        case LONG_INT: tkn->long_v = *(long*)value; break;
        default: tkn->string_v = value; break;
    }

    return;
}
