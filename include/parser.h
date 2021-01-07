
#include "ast.h"
#include "token.h"



// tkn_vec (arg)
// stmt_vector => to store ast of individual stmts
// only pass stmt_vector to parse_stmts.

struct stmt_vector* parse (struct token_vector* tkn_vec);

void parse_decl (struct token_vector* tkn_vec, struct stmt_vector* program);
void parse_var_decl (struct token_vector* tkn_vec, struct stmt_vector* program);

void parse_stmt (struct token_vector* tkn_vec, struct stmt_vector* program);
void parse_return_stmt (struct token_vector* tkn_vec, struct stmt_vector* program);
void parse_while_stmt (struct token_vector* tkn_vec, struct stmt_vector* program);
void parse_if_stmt (struct token_vector* tkn_vec, struct stmt_vector* program);
void parse_block_stmt (struct token_vector* tkn_vec, struct stmt_vector* program);
void parse_instructn_stmt (struct token_vector* tkn_vec, struct stmt_vector* program);
void parse_assign_stmt (struct token_vector* tkn_vec, struct stmt_vector* program);
void parse_output_stmt (struct token_vector* tkn_vec, struct stmt_vector* program);

struct expr* parse_expr (struct token_vector* tkn_vec);
struct expr* parse_or_expr (struct token_vector* tkn_vec);
struct expr* parse_and_expr (struct token_vector* tkn_vec);
struct expr* parse_equality_expr (struct token_vector* tkn_vec);
struct expr* parse_comparison_expr (struct token_vector* tkn_vec);
struct expr* parse_term_expr (struct token_vector* tkn_vec);
struct expr* parse_factor_expr (struct token_vector* tkn_vec);
struct expr* parse_unary_expr (struct token_vector* tkn_vec);
struct expr* parse_call_expr (struct token_vector* tkn_vec);
struct expr* parse_primary_expr (struct token_vector* tkn_vec);
