#ifndef AST_H
#define AST_H

#include "token.h"
#define MAX_ARGS 12


enum expr_type { LITERAL, PAREN, BINARY, UNARY, VARIABLE, CALL };

struct expr
{
    enum expr_type type;
    union
    {
        struct literal_expr* literal;
        struct paren* grouping;
        struct binary_expr* binary;
        struct unary_expr* unary;
        struct variable_expr* variable;
        struct call_expr* call;
    };
};

struct literal_expr
{
    struct token* info;
};

struct paren
{
    struct expr* expr;
};

struct binary_expr
{
    struct expr* left;
    struct token* op;
    struct expr* right;
};

struct unary_expr
{
    struct token* op;
    struct expr* right;
};

struct variable_expr
{
    struct token* id;
};

struct arg_vector;
struct call_expr
{
    struct expr* id;
    struct arg_vector* args;
};

struct arg_vector
{
    int n_args;
    struct expr** vec;
};

struct arg_vector* construct_arg_vector();
void insert_arg (struct expr* expr, struct arg_vector* arg_vec); 



enum stmt_type { VAR_DECL, BLOCK, IF_STMT, WHILE_STMT, FUNCTION_STMT, RETURN_STMT, ASSIGN, OUTPUT_STMT }; 
// expr_stmt? depends on if language has structs and side effects.

struct stmt
{
    enum stmt_type type;
    union
    {
        struct var_decl* decl_stmt;
        struct if_stmt* if_stmt;
        struct while_stmt* while_stmt;
        struct return_stmt* return_stmt;
        struct assign_stmt* assign_stmt;
        struct block_stmt* block_stmt;
        struct output_stmt* output_stmt;
        struct function_stmt* function;
    };
};

// think about adding structs.
struct var_decl
{
    struct token* info;  // i.e. type-info
    struct token* id;
    struct expr* value;
};

struct if_stmt
{
    struct expr* cond;
    struct stmt* then_branch;
    struct stmt* else_branch;
};

struct while_stmt
{
    struct expr* cond;
    struct stmt* body;
};

struct return_stmt
{
    struct expr* ret_v;
};

struct assign_stmt
{
    struct token* id;
    struct expr* value;
};

struct output_stmt
{
    struct expr* output_v;
};

struct stmt_vector;
struct block_stmt
{
    struct stmt_vector* stmts;
};

struct param_vector;
struct function_stmt
{
    struct token* info;  // i.e return-type
    struct token* id; 
    struct param_vector* params;
    struct stmt* body;
};

struct stmt_vector
{
    int n_stmts;
    int max_length;
    struct stmt** vec;
};

struct stmt_vector* construct_stmt_vector();
void insert_stmt (struct stmt* parsed_stmt, struct stmt_vector* stmt_vec); 

struct param_vector
{ 
    int n_params;
    struct stmt** vec; 
};

struct param_vector* construct_param_vector();
void insert_param (struct stmt* stmt, struct param_vector* param_vec); 

#endif
