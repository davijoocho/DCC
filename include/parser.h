#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "token.h"

struct stmt_vector* parse (struct token_vector* tkn_vec);

struct stmt* parse_decl (struct token_vector* tkn_vec);
struct stmt* parse_var_decl (struct token_vector* tkn_vec);

struct stmt* parse_stmt (struct token_vector* tkn_vec);
struct stmt* parse_return_stmt (struct token_vector* tkn_vec);
struct stmt* parse_while_stmt (struct token_vector* tkn_vec);
struct stmt* parse_if_stmt (struct token_vector* tkn_vec);
struct stmt* parse_block_stmt (struct token_vector* tkn_vec);
struct stmt* parse_instructn_stmt (struct token_vector* tkn_vec);
struct stmt* parse_assign_stmt (struct token_vector* tkn_vec);
struct stmt* parse_output_stmt (struct token_vector* tkn_vec);

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


struct expr* construct_binary_expr (struct expr* left_expr, struct token* op_tkn, struct expr* right_expr);

void parse_binary_expr (struct expr* ast, struct token_vector* tkn_vec, 
        struct expr* (*parse_fn)(struct token_vector*) );

struct stmt* construct_stmt (enum stmt_type tag, void* stmt);

#endif
