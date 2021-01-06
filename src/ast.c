
#include "ast.h"


struct arg_vector* construct_arg_vector()
{
    struct arg_vector* arg_vec = malloc(sizeof(struct arg_vector));
    arg_vec->n_args = 0;
    arg_vec->vec = malloc( sizeof(struct expr) * MAX_ARGS );
    return arg_vec;
}
    // add error handling for cond: n_exprs >= MAX_ARGS
void insert_arg (struct expr* expr, struct arg_vector* arg_vec) { if (arg_vec->n_args != MAX_ARGS) arg_vec->vec[arg_vec->n_args++] = expr; }


struct stmt_vector* construct_stmt_vector()
{
    struct stmt_vector* stmt_vec = malloc(sizeof(struct stmt_vector));
    stmt_vec->n_stmts = 0;
    stmt_vec->max_length = 20;
    stmt_vec->vec = malloc( sizeof(struct stmt) * 20 );
    return stmt_vec;
}

void add_stmt (struct stmt* stmt, struct stmt_vector* stmt_vec)
{
    if (stmt_vec->n_stmts >= stmt_vec->max_length) stmt_vec->vec = realloc(stmt_vec->vec, sizeof(struct stmt) * stmt_vec->max_length *= 2);
    stmt_vec->vec[stmt_vec->n_stmts++] = stmt;
};



struct param_vector* construct_param_vector()
{
    struct param_vector* param_vec = malloc(sizeof(struct param_vector));
    param_vec->n_params = 0;
    param_vec->vec = malloc( sizeof(struct token) * MAX_ARGS );
    return param_vec;
}
    // add error handling for cond: n_params >= MAX_ARGS
void add_params (struct token* tkn, struct param_vector* param_vec) { if (param_vec->n_params != MAX_ARGS) param_vec->vec[param_vec->n_params++] = tkn; }


