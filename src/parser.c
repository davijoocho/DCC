#include "ast.h"
#include "parser.h"

struct stmt_vector* parse (struct token_vector* tkn_vec)
{
    struct stmt_vector* program = construct_stmt_vector();
    while (tkn_vec->vec[tkn_vec->pos] != EOF_F) 
        parse_decl(tkn_vec, program);
    return program;
}

void parse_decl (struct token_vector* tkn_vec, struct stmt_vector* program)
{
    // fut_ref -> error handle: catch exception if error occurred during descent (synchronize, setjmp, longjmp)
    enum token_type e;
    for (e = 22; i < 24; i++) {
        if (tkn_vec->vec[tkn_vec->pos].type == e && tkn_vec->pos++)
            parse_var_decl(tkn_vec, program);
    } 
    parse_stmt(tkn_vec, program);
    return;
}

void parse_var_decl (struct token_vector* tkn_vec, struct stmt_vector* program)
{
    struct token* var_type = &tkn_vec->vec[tkn_vec->pos-1];
    struct token* var_id = &tkn_vec->vec[tkn_vec->pos++];   // fut_ref -> error handle: if not id (expect)
    struct expr* init_v = NULL;

    if (tkn_vec->vec[tkn_vec->pos].type == EQUAL && tkn_vec->pos++) 
        init_v = parse_expr(tkn_vec);

    tkn_vec->pos++; // fut_ref -> error handle: if not semicolon (expect);

    struct var_decl* var_stmt = malloc(sizeof(struct var_decl));
    var_stmt->info = var_type; var_stmt->id = var_id; var_stmt->value = init_v;

    insert_stmt(VAR_DECL, var_stmt, program); 
    return;
}


struct expr* parse_expr (struct token_vector* tkn_vec) {}





