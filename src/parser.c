#include <stdio.h>
#include "ast.h"
#include "parser.h"

struct stmt_vector* parse (struct token_vector* tkn_vec)
{
    printf("parse\n");
    struct stmt_vector* program = construct_stmt_vector();

    while (tkn_vec->vec[tkn_vec->pos].type != EOF_F)
        insert_stmt(parse_decl(tkn_vec), program);

    return program;
}

struct stmt* parse_decl (struct token_vector* tkn_vec)
{
    // fut_ref -> error handle: catch exception if error occurred during descent (synchronize, setjmp, longjmp)
    printf("parse_decl\n");

    enum token_type e;
    for (e = 22; e < 24; e++) 
        if (tkn_vec->vec[tkn_vec->pos].type == e && ++tkn_vec->pos) 
            return parse_var_decl(tkn_vec);

    return parse_stmt(tkn_vec);
}

struct stmt* parse_var_decl (struct token_vector* tkn_vec)
{
    printf("parse_var_decl\n");
    struct token* var_type = &tkn_vec->vec[tkn_vec->pos-1];
    struct token* var_id = &tkn_vec->vec[tkn_vec->pos++];   // fut_ref -> error handle: if not id (expect)
    struct expr* init_v = NULL;

    if (tkn_vec->vec[tkn_vec->pos].type == EQUAL && tkn_vec->pos++) 
        init_v = parse_expr(tkn_vec);
    tkn_vec->pos++; // fut_ref -> error handle: if not semicolon (expect);

    struct var_decl* var_stmt = malloc(sizeof(struct var_decl));
    var_stmt->info = var_type; var_stmt->id = var_id; var_stmt->value = init_v;

    return construct_stmt(VAR_DECL, var_stmt);
}


struct stmt* parse_stmt (struct token_vector* tkn_vec)
{
    printf("parse_stmt\n");
    switch (tkn_vec->vec[tkn_vec->pos++].type) {
        case RETURN: 
            return parse_return_stmt(tkn_vec);
        case WHILE: 
            return parse_while_stmt(tkn_vec);
        case IF: 
            return parse_if_stmt(tkn_vec); 
        case LEFT_BRACE: 
            return parse_block_stmt(tkn_vec); 
        case INSTRUCTN: 
            return parse_instructn_stmt(tkn_vec); 
        case OUTPUT: 
            return parse_output_stmt(tkn_vec); 
        case IDENTIFIER:
            return parse_assign_stmt(tkn_vec);
        default:   // fut_ref -> error handle
            break;
    }

    return NULL;
}


struct stmt* parse_return_stmt (struct token_vector* tkn_vec)
{
    printf("parse_return_stmt\n");
    struct return_stmt* ret_stmt = malloc(sizeof(struct return_stmt));
    ret_stmt->ret_v = NULL;

    if (tkn_vec->vec[tkn_vec->pos].type != SEMICOLON)
        ret_stmt->ret_v = parse_expr(tkn_vec);
    tkn_vec->pos++;      // fut_ref -> error handle: expect semicolon.

    return construct_stmt(RETURN_STMT, ret_stmt);
}

struct stmt* parse_block_stmt (struct token_vector* tkn_vec)
{
    printf("parse_block_stmt\n");
    struct stmt_vector* b_stmts = construct_stmt_vector();

    while (tkn_vec->vec[tkn_vec->pos].type != RIGHT_BRACE)   // fut_ref -> error handle: check for EOF.
        insert_stmt( parse_decl(tkn_vec) ,b_stmts);
    tkn_vec->pos++;     // fut_ref -> error handle: expect right brace.

    struct block_stmt* block = malloc(sizeof(struct block_stmt));
    block->stmts = b_stmts;

    return construct_stmt(BLOCK, block);
}

struct stmt* parse_while_stmt (struct token_vector* tkn_vec)
{
    printf("parse_while_stmt\n");
    struct while_stmt* wh_stmt = malloc(sizeof(struct while_stmt));
    
    tkn_vec->pos++;    // fut_ref -> error handle: expect left paren.
    wh_stmt->cond = parse_expr(tkn_vec);
    tkn_vec->pos++;    // fut_ref -> error handle: expect right paren.

    wh_stmt->body = parse_stmt(tkn_vec);

    return construct_stmt(WHILE_STMT, wh_stmt);
}


struct stmt* parse_if_stmt (struct token_vector* tkn_vec)
{
    printf("parse_if_stmt\n");
    struct if_stmt* stmt = malloc(sizeof(struct if_stmt));
    
    tkn_vec->pos++;    // fut_ref -> error handle: expect left paren.
    stmt->cond = parse_expr(tkn_vec);
    tkn_vec->pos++;    // fut_ref -> error handle: expect right paren.

    stmt->then_branch = parse_stmt(tkn_vec);
    stmt->else_branch = NULL;

    if (tkn_vec->vec[tkn_vec->pos].type == ELSE && tkn_vec->pos++)
        stmt->else_branch = parse_stmt(tkn_vec);

    return construct_stmt(IF_STMT, stmt);
}

struct stmt* parse_output_stmt (struct token_vector* tkn_vec)
{
    printf("parse_output_stmt\n");
    struct output_stmt* out_stmt = malloc(sizeof(struct output_stmt));

    tkn_vec->pos++;    // fut_ref -> error handle: expect left paren.
    out_stmt->output_v = parse_expr(tkn_vec);
    tkn_vec->pos++;    // fut_ref -> error handle: expect right paren.
    tkn_vec->pos++;    // fut_ref -> error handle: expect semicolon.

    return construct_stmt(OUTPUT_STMT, out_stmt);
}

struct stmt* parse_assign_stmt (struct token_vector* tkn_vec)
{
    printf("parse_assign_stmt\n");
    struct assign_stmt* eq_stmt = malloc(sizeof(struct assign_stmt));

    eq_stmt->id = &tkn_vec->vec[tkn_vec->pos-1];
    tkn_vec->pos++;    // fut_ref -> error handle: expect an assignment '=' operator.

    eq_stmt->value = parse_expr(tkn_vec);
    tkn_vec->pos++;    // fut_ref -> error handle: expect a semicolon.

    return construct_stmt(ASSIGN, eq_stmt);
}

struct stmt* parse_instructn_stmt (struct token_vector* tkn_vec)
{
    printf("parse_instructn_stmt\n");
    struct instructn* fn = malloc(sizeof(struct instructn));
    fn->id = &tkn_vec->vec[tkn_vec->pos++];

    tkn_vec->pos++;  // fut_ref -> error handle: expect a left paren.

    if (tkn_vec->vec[tkn_vec->pos].type != RIGHT_PAREN) {
        fn->params = construct_param_vector();
        do {
            insert_param(&tkn_vec->vec[tkn_vec->pos++], fn->params);
        } while (tkn_vec->vec[tkn_vec->pos].type == COMMA && tkn_vec->pos++); 
    } 

    tkn_vec->pos++;   // fut_ref -> error handle: expect a RIGHT_PAREN.
    tkn_vec->pos++;   // fut_ref -> error handle: expect an ARROW '=>'.

    fn->info = &tkn_vec->vec[tkn_vec->pos++];
    fn->body = parse_stmt(tkn_vec);

    return construct_stmt(INSTRUCTN_STMT, fn);
}



// fut_ref -> parser functions for binary expressions are repetitive 
// fut_ref -> parse by precedence by maintaining precedence table and function pointers (no need for multiple functions).

struct expr* parse_expr (struct token_vector* tkn_vec) { printf("parse_expr\n"); return parse_or_expr(tkn_vec); }

struct expr* parse_or_expr (struct token_vector* tkn_vec) 
{
    printf("parse_or_expr\n");
    struct expr* ast = parse_and_expr(tkn_vec);

    while (tkn_vec->vec[tkn_vec->pos].type == OR)
        parse_binary_expr(ast, tkn_vec, &parse_and_expr);
    return ast;
}

struct expr* parse_and_expr (struct token_vector* tkn_vec)
{
    printf("parse_and_expr\n");
    struct expr* ast = parse_equality_expr(tkn_vec);

    while (tkn_vec->vec[tkn_vec->pos].type == AND)
        parse_binary_expr(ast, tkn_vec, &parse_equality_expr);
    return ast;
}

struct expr* parse_equality_expr (struct token_vector* tkn_vec) 
{
    printf("parse_equality_expr\n");
    struct expr* ast = parse_comparison_expr(tkn_vec);
    enum token_type e;

    for (e = 12; e < 14; e++)
        if (tkn_vec->vec[tkn_vec->pos].type == e) {
            parse_binary_expr(ast, tkn_vec, &parse_comparison_expr);
            e = 12; 
        }
    return ast;
}

struct expr* parse_comparison_expr (struct token_vector* tkn_vec)
{
    printf("parse_comparison_expr\n");
    struct expr* ast = parse_term_expr(tkn_vec);
    enum token_type e;

    for (e = 14; e < 18; e++) 
        if (tkn_vec->vec[tkn_vec->pos].type == e) {
            parse_binary_expr(ast, tkn_vec, &parse_term_expr);
            e = 14;
        }
    return ast;
}

struct expr* parse_term_expr (struct token_vector* tkn_vec)
{
    printf("parse_term_expr\n");
    struct expr* ast = parse_factor_expr(tkn_vec);
    enum token_type e;

    for (e = 4; e < 6; e++) 
        if (tkn_vec->vec[tkn_vec->pos].type == e) {
            parse_binary_expr(ast, tkn_vec, &parse_factor_expr);
            e = 4;
        }
    return ast;
}

struct expr* parse_factor_expr (struct token_vector* tkn_vec)
{
    printf("parse_factor_expr\n");
    struct expr* ast = parse_unary_expr(tkn_vec);
    enum token_type e;

    for (e = 8; e < 10; e++) 
        if (tkn_vec->vec[tkn_vec->pos].type == e) {
            parse_binary_expr(ast, tkn_vec, &parse_unary_expr);
            e = 8;
        }
    return ast;
}


struct expr* parse_unary_expr (struct token_vector* tkn_vec)
{
    printf("parse_unary_expr\n");
    if (tkn_vec->vec[tkn_vec->pos].type == BANG || tkn_vec->vec[tkn_vec->pos].type == MINUS) {
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
    printf("parse_call_expr\n");
    struct expr* ast = parse_primary_expr(tkn_vec);

    if (tkn_vec->vec[tkn_vec->pos].type == LEFT_PAREN && tkn_vec->pos++) {
        struct arg_vector* args_v = construct_arg_vector();

        if (tkn_vec->vec[tkn_vec->pos].type != RIGHT_PAREN)
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
    printf("parse_primary_expr\n");
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

void parse_binary_expr (struct expr* ast, struct token_vector* tkn_vec, 
        struct expr* (*parse_fn)(struct token_vector*) )
{
    struct token* operator = &tkn_vec->vec[tkn_vec->pos++];
    struct expr* r_ast = parse_fn(tkn_vec);
    ast = construct_binary_expr(ast, operator, r_ast);
    return;
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



