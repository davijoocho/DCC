
#include "token.h"


enum expr_type { LITERAL, PAREN, BINARY, UNARY, VARIABLE, CALL };

struct expr
{
    enum expr_type type;
    union
    {
        struct literal_expr* literal_expr;
        struct paren* paren;
        struct binary_expr* binary_expr;
        struct unary_expr* unary_expr;
        struct variable_expr* variable_expr;
        struct call_expr* call_expr;
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
    enum token_type op;
    struct expr* right;
};

struct variable_expr
{
    struct token* id;
};

struct expr_vector;
struct call_expr
{
    struct token* id;
    struct expr_vector* args;
};

struct expr_vector
{
    int n_exprs;
    int max_length;  // defined as 12 for now.
    struct expr* vec;
};

struct expr_vector* construct_expr_vec();
void add_expr (struct expr* expr, struct expr_vector* expr_vec); 


enum stmt_type { VAR_DECL, BLOCK, IF, WHILE, INSTRUCTN, RETURN, ASSIGN }; 
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
        struct instructn* instructn;
    };
};

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

struct stmt_vector;
struct block_stmt
{
    struct stmt_vector* stmts;
};

struct param_vector;
struct instructn
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
    struct stmt* vec;
};

struct stmt_vector* construct_stmt_vector();
void add_stmt (struct stmt* stmt, struct stmt_vector* stmt_vec); // realloc when nstmts = max_length 


struct param_vector
{ 
    int n_params;
    int max_length;  // defined as 12 for now.
    struct token* vec;
};

struct param_vector* construct_param_vector();
void add_param (struct token* tkn, struct param_vector* param_vec); 





