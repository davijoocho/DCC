#include <stdio.h>
#include "ast.h"
#include "parser.h"

struct stmt_vector* parse (struct token_vector* tkn_vec)
{
    struct stmt_vector* program = construct_stmt_vector();

    while (!match_op(tkn_vec, 1, EOF_F))
        insert_stmt(parse_decl(tkn_vec), program);

    return program;
}

struct stmt* parse_decl (struct token_vector* tkn_vec)
{
    // fut_ref -> error handle: catch exception if error occurred during descent (synchronize, setjmp, longjmp)
    if (match_op(tkn_vec, 2, INT, LONG) && ++tkn_vec->pos) 
        return parse_var_decl(tkn_vec);

    return parse_stmt(tkn_vec);
}

struct stmt* parse_var_decl (struct token_vector* tkn_vec)
{
    struct token* var_type = &tkn_vec->vec[tkn_vec->pos-1];
    struct token* var_id = &tkn_vec->vec[tkn_vec->pos++];   // fut_ref -> error handle: if not id (expect)
    struct expr* init_v = NULL;

    if (match_op(tkn_vec, 1, EQUAL) && tkn_vec->pos++) 
        init_v = parse_expr(tkn_vec);
    tkn_vec->pos++; // fut_ref -> error handle: if not semicolon (expect);

    struct var_decl* var_stmt = malloc(sizeof(struct var_decl));
    var_stmt->info = var_type; var_stmt->id = var_id; var_stmt->value = init_v;

    return construct_stmt(VAR_DECL, var_stmt);
}


struct stmt* parse_stmt (struct token_vector* tkn_vec)
{
    switch (tkn_vec->vec[tkn_vec->pos++].type) {
        case RETURN: return parse_return_stmt(tkn_vec);
        case WHILE: return parse_while_stmt(tkn_vec);
        case IF: return parse_if_stmt(tkn_vec); 
        case LEFT_BRACE: return parse_block_stmt(tkn_vec); 
        case INSTRUCTN: return parse_instructn_stmt(tkn_vec); 
        case OUTPUT: return parse_output_stmt(tkn_vec); 
        case IDENTIFIER: return parse_assign_stmt(tkn_vec);
        default:   // fut_ref -> error handle
            break;
    }

    return NULL;
}


struct stmt* parse_return_stmt (struct token_vector* tkn_vec)
{
    struct return_stmt* ret_stmt = malloc(sizeof(struct return_stmt));
    ret_stmt->ret_v = NULL;

    if (!match_op(tkn_vec, 1, SEMICOLON))
        ret_stmt->ret_v = parse_expr(tkn_vec);
    tkn_vec->pos++;      // fut_ref -> error handle: expect semicolon.

    return construct_stmt(RETURN_STMT, ret_stmt);
}

struct stmt* parse_block_stmt (struct token_vector* tkn_vec)
{
    struct stmt_vector* b_stmts = construct_stmt_vector();

    while (!match_op(tkn_vec, 1, RIGHT_BRACE))   // fut_ref -> error handle: check for EOF.
        insert_stmt( parse_decl(tkn_vec) ,b_stmts);
    
    tkn_vec->pos++;     // fut_ref -> error handle: expect right brace.

    struct block_stmt* block = malloc(sizeof(struct block_stmt));
    block->stmts = b_stmts;

    return construct_stmt(BLOCK, block);
}

struct stmt* parse_while_stmt (struct token_vector* tkn_vec)
{
    struct while_stmt* wh_stmt = malloc(sizeof(struct while_stmt));
    
    tkn_vec->pos++;    // fut_ref -> error handle: expect left paren.
    wh_stmt->cond = parse_expr(tkn_vec);
    tkn_vec->pos++;    // fut_ref -> error handle: expect right paren.

    wh_stmt->body = parse_stmt(tkn_vec);

    return construct_stmt(WHILE_STMT, wh_stmt);
}


struct stmt* parse_if_stmt (struct token_vector* tkn_vec)
{
    struct if_stmt* stmt = malloc(sizeof(struct if_stmt));
    
    tkn_vec->pos++;    // fut_ref -> error handle: expect left paren.
    stmt->cond = parse_expr(tkn_vec);
    tkn_vec->pos++;    // fut_ref -> error handle: expect right paren.

    stmt->then_branch = parse_stmt(tkn_vec);
    stmt->else_branch = NULL;

    if (match_op(tkn_vec, 1, ELSE) && tkn_vec->pos++)
        stmt->else_branch = parse_stmt(tkn_vec);

    return construct_stmt(IF_STMT, stmt);
}

struct stmt* parse_output_stmt (struct token_vector* tkn_vec)
{
    struct output_stmt* out_stmt = malloc(sizeof(struct output_stmt));

    tkn_vec->pos++;    // fut_ref -> error handle: expect left paren.
    out_stmt->output_v = parse_expr(tkn_vec);
    tkn_vec->pos++;    // fut_ref -> error handle: expect right paren.
    tkn_vec->pos++;    // fut_ref -> error handle: expect semicolon.

    return construct_stmt(OUTPUT_STMT, out_stmt);
}

struct stmt* parse_assign_stmt (struct token_vector* tkn_vec)
{
    struct assign_stmt* eq_stmt = malloc(sizeof(struct assign_stmt));

    eq_stmt->id = &tkn_vec->vec[tkn_vec->pos-1];
    tkn_vec->pos++;    // fut_ref -> error handle: expect an assignment '=' operator.

    eq_stmt->value = parse_expr(tkn_vec);
    tkn_vec->pos++;    // fut_ref -> error handle: expect a semicolon.

    return construct_stmt(ASSIGN, eq_stmt);
}

struct stmt* parse_instructn_stmt (struct token_vector* tkn_vec)
{
    struct instructn* fn = malloc(sizeof(struct instructn));
    fn->id = &tkn_vec->vec[tkn_vec->pos++];

    tkn_vec->pos++;  // fut_ref -> error handle: expect a left paren.

    if (!match_op(tkn_vec, 1, RIGHT_PAREN)) {
        fn->params = construct_param_vector();
        do {
            struct var_decl* param = malloc(sizeof(struct var_decl));
            param->info = &tkn_vec->vec[tkn_vec->pos++];
            param->id = &tkn_vec->vec[tkn_vec->pos++];
            param->value = NULL;

            insert_param( construct_stmt(VAR_DECL, param) , fn->params);
        } while (match_op(tkn_vec, 1, COMMA) && tkn_vec->pos++); 
    } else {
        fn->params = NULL;
    }


    tkn_vec->pos++;   // fut_ref -> error handle: expect a RIGHT_PAREN.
    tkn_vec->pos++;   // fut_ref -> error handle: expect an ARROW '=>'.

    fn->info = &tkn_vec->vec[tkn_vec->pos++];   // fut_ref -> error handle: expect a function return type 
    fn->body = parse_stmt(tkn_vec);

    return construct_stmt(INSTRUCTN_STMT, fn);
}



// fut_ref -> parser functions for binary expressions are repetitive 
// fut_ref -> parse by precedence by maintaining precedence table and function pointers (no need for multiple functions).

struct expr* parse_expr (struct token_vector* tkn_vec) { 
    return parse_or_expr(tkn_vec); 
}

struct expr* parse_or_expr (struct token_vector* tkn_vec) 
{
    struct expr* ast = parse_and_expr(tkn_vec);

    while (match_op(tkn_vec, 1, OR))
        ast = parse_binary_expr(ast, tkn_vec, &parse_and_expr);
    return ast;
}

struct expr* parse_and_expr (struct token_vector* tkn_vec)
{
    struct expr* ast = parse_equality_expr(tkn_vec);

    while (match_op(tkn_vec, 1, AND))
        ast = parse_binary_expr(ast, tkn_vec, &parse_equality_expr);
    return ast;
}

struct expr* parse_equality_expr (struct token_vector* tkn_vec) 
{
    struct expr* ast = parse_comparison_expr(tkn_vec);
    
    while (match_op(tkn_vec, 2, BANG_EQUAL, EQUAL_EQUAL))
        ast = parse_binary_expr(ast, tkn_vec, &parse_comparison_expr);

    return ast;
}

struct expr* parse_comparison_expr (struct token_vector* tkn_vec)
{
    struct expr* ast = parse_term_expr(tkn_vec);
    
    while (match_op(tkn_vec, 4, LESS_EQUAL, LESS, GREATER, GREATER_EQUAL))
        ast = parse_binary_expr(ast, tkn_vec, &parse_term_expr);

    return ast;
}

struct expr* parse_term_expr (struct token_vector* tkn_vec)
{
    struct expr* ast = parse_factor_expr(tkn_vec);

    while (match_op(tkn_vec, 2, PLUS, MINUS))
        ast = parse_binary_expr(ast, tkn_vec, &parse_factor_expr);

    return ast;
}

struct expr* parse_factor_expr (struct token_vector* tkn_vec)
{
    struct expr* ast = parse_unary_expr(tkn_vec);

    while(match_op(tkn_vec, 2, ASTERISK, SLASH))
        ast = parse_binary_expr(ast, tkn_vec, &parse_unary_expr);

    return ast;
}


struct expr* parse_unary_expr (struct token_vector* tkn_vec)
{
    if (match_op(tkn_vec, 2, BANG, MINUS)) {
        struct token* operator = &tkn_vec->vec[tkn_vec->pos++];
        struct expr* r_ast = parse_unary_expr(tkn_vec);

        struct unary_expr* un_expr = malloc(sizeof(struct unary_expr));
        un_expr->op = operator;  un_expr->right = r_ast;

        struct expr* parsed_expr = malloc(sizeof(struct expr));
        parsed_expr->type = UNARY;  parsed_expr->unary = un_expr;

        return parsed_expr;
    } 

    return parse_call_expr(tkn_vec);
}


struct expr* parse_call_expr (struct token_vector* tkn_vec)
{
    struct expr* ast = parse_primary_expr(tkn_vec);

    if (match_op(tkn_vec, 1, LEFT_PAREN) && tkn_vec->pos++) {
        struct arg_vector* args_v = construct_arg_vector();

        if (!match_op(tkn_vec, 1, RIGHT_PAREN))
            do 
                insert_arg( parse_expr(tkn_vec), args_v );        // fut_ref -> add error handling for exceeding max args.
            while (tkn_vec->vec[tkn_vec->pos].type == COMMA && tkn_vec->pos++);

        tkn_vec->pos++; // fut_ref-> add error handling for expecting right parenthesis (expect).
        
        struct call_expr* fn_c = malloc(sizeof(struct call_expr));
        fn_c->id = ast;  fn_c->args = args_v;

        struct expr* parsed_expr = malloc(sizeof(struct expr));
        parsed_expr->type = CALL;  parsed_expr->call = fn_c;

        ast = parsed_expr;
    }

    return ast;
}

struct expr* parse_primary_expr (struct token_vector* tkn_vec)
{
    struct expr* p_expr = malloc(sizeof(struct expr));

    switch (tkn_vec->vec[tkn_vec->pos].type) {
        case LONG_INTEGER: {
            struct literal_expr* lit_expr = malloc(sizeof(struct literal_expr));
            lit_expr->info = &tkn_vec->vec[tkn_vec->pos++];
            p_expr->type = LITERAL;
            p_expr->literal = lit_expr;
            return p_expr;
        } 
        case INTEGER: {
            struct literal_expr* lit_expr = malloc(sizeof(struct literal_expr));
            lit_expr->info = &tkn_vec->vec[tkn_vec->pos++];
            p_expr->type = LITERAL;
            p_expr->literal = lit_expr;
            return p_expr;
        }

        case IDENTIFIER: {
            struct variable_expr* var_expr = malloc(sizeof(struct variable_expr));
            var_expr->id = &tkn_vec->vec[tkn_vec->pos++];
            p_expr->type = VARIABLE;
            p_expr->variable = var_expr;
            return p_expr;
        }
        case LEFT_PAREN: {
            struct paren* group = malloc(sizeof(struct paren));
            tkn_vec->pos++;
            group->expr = parse_expr( tkn_vec );
            p_expr->type = PAREN;
            p_expr->grouping = group;
            tkn_vec->pos++; // fut_ref -> add error handle for unmatched parenthesis (expect)
            return p_expr;
        }
        default:
            break; // fut_ref -> add error handle for expecting an expression.
    }

    return NULL;
}


struct expr* construct_binary_expr (struct expr* left_expr, struct token* op_tkn, struct expr* right_expr) 
{
    struct binary_expr* bi_expr = malloc(sizeof(struct binary_expr));
    bi_expr->left = left_expr;   
    bi_expr->op = op_tkn;   
    bi_expr->right = right_expr;

    struct expr* parsed_expr = malloc(sizeof(struct expr));
    parsed_expr->type = BINARY;  
    parsed_expr->binary = bi_expr;
    return parsed_expr;
}

struct expr* parse_binary_expr (struct expr* ast, struct token_vector* tkn_vec, 
        struct expr* (*parse_fn)(struct token_vector*) )
{
    struct token* operator = &tkn_vec->vec[tkn_vec->pos++]; 
    struct expr* r_ast = parse_fn(tkn_vec);
    return construct_binary_expr(ast, operator, r_ast);
}

struct stmt* construct_stmt (enum stmt_type tag, void* stmt)
{
    struct stmt* p_stmt = malloc(sizeof(struct stmt));
    p_stmt->type = tag;

    switch (tag) {
        case VAR_DECL: p_stmt->decl_stmt = (struct var_decl*)stmt; break;
        case BLOCK: p_stmt->block_stmt = (struct block_stmt*)stmt; break;
        case IF_STMT: p_stmt->if_stmt = (struct if_stmt*)stmt; break;
        case WHILE_STMT: p_stmt->while_stmt = (struct while_stmt*)stmt; break;
        case INSTRUCTN_STMT: p_stmt->instructn = (struct instructn*)stmt; break;
        case RETURN_STMT: p_stmt->return_stmt = (struct return_stmt*)stmt; break;
        case ASSIGN: p_stmt->assign_stmt = (struct assign_stmt*)stmt; break;
        case OUTPUT_STMT: p_stmt->output_stmt = (struct output_stmt*)stmt; break;
    } 
    return p_stmt;
}

bool match_op (struct token_vector* tkn_vec, int n, ...)
{

    int i;
    va_list op_types;
    va_start(op_types, n);
    for (i=0; i<n; i++) {
        enum token_type e = va_arg(op_types, int);
        if (tkn_vec->vec[tkn_vec->pos].type == e) {
            va_end(op_types);
            return true;
        }
    }
    va_end(op_types);
    return false;
}
