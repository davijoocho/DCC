#include <stdlib.h>
#include <stdio.h>
#include "parser.h"

struct program* parse(struct tokens* toks) {
   struct program* prog = malloc(sizeof(struct program)); 
   INIT_PROGRAM(prog);

   while (NOT_EOF(toks)) {
       if (NEXT_TOKEN(toks) == SPACES || NEXT_TOKEN(toks) == NEW_LINE) {
           CONSUME_TOKEN(toks);
       } else {
           if (OVERFLOW(prog)) {
               GROW_MEMORY(prog);
           }
           //printf("parse()\n");
           ADD_STATEMENT(prog, parse_stmt(toks, 0));
       }
   }

   if (UNUSED_MEMORY_EXISTS(prog)) {
       SHRINK_MEMORY(prog);
   }

   return prog;
}


struct stmt* parse_stmt(struct tokens* toks, int scope) {
    struct stmt* stmt = malloc(sizeof(struct stmt));

    if (NEXT_TOKEN(toks) == IDENTIFIER) {
        if (LOOK_AHEAD(toks) == LEFT_PAREN) {
            //printf("parse_stmt() -> PROCEDURE CALL\n");
            INIT_PROCEDURE_CALL(stmt, toks);

            while (NEXT_TOKEN(toks) != RIGHT_PAREN) {
                INSERT_ARG(stmt->proc, parse_expr(toks, 0));
                if (NEXT_TOKEN(toks) == COMMA) {
                    CONSUME_TOKEN(toks); // expect ','
                }
            }
            CONSUME_TOKEN(toks);  // expect ')'
            CONSUME_TOKEN(toks);  // expect '\n'

            if (NOT_MAX_ARGS(stmt->proc)) {
                SHRINK_ARGS(stmt->proc);
            }

        } else if (LOOK_AHEAD(toks) == ASSIGN) {
            //printf("parse_stmt() -> ASSIGN\n");
            INIT_ASSIGN_STMT(stmt, toks);
            CONSUME_TOKEN(toks);   // expect '\n'
        }

    } else if (NEXT_TOKEN_IS_TYPE(toks)) {
        //printf("parse_stmt() -> VAR DECL\n");
        INIT_VAR_DECL(stmt, toks, scope);
        if (scope != 0) {            // NOT FUNCTION PARAMETER
            CONSUME_TOKEN(toks);   // expect '\n'
        }

    } else if (NEXT_TOKEN(toks) == RETURN) {
        //printf("print_stmt() -> RETURN\n");
        INIT_RETURN_STMT(stmt, toks);
        CONSUME_TOKEN(toks);   // expect '\n'

    } else if (NEXT_TOKEN(toks) == COLON) {
        //printf("print_stmt() -> BLOCK scope %d\n", scope / 4);
        CONSUME_TOKEN(toks);   // expect ':'
        CONSUME_TOKEN(toks);   // expect '\n'
        INIT_BLOCK_STMT(stmt, toks);
        int blk_capacity = 64;

        while (NOT_EOF(toks) && (NEXT_TOKEN(toks) == SPACES || NEXT_TOKEN(toks) == NEW_LINE)) {
            if (NEXT_TOKEN(toks) == NEW_LINE) {
                CONSUME_TOKEN(toks);  // expect '\n'
            } else  {
                if (LOOK_AHEAD(toks) == NEW_LINE) { 
                    SKIP_LINE(toks);
                } else {
                    if (SPACE_INDENT(toks) % 4 != 0) {
                        // error
                    } else {
                        if (SPACE_INDENT(toks) > scope) {
                            // error
                        } else if (SPACE_INDENT(toks) == scope) {
                            CONSUME_TOKEN(toks);  // expect ' '
                            if (BLOCK_OVERFLOW(stmt->block, blk_capacity)) {
                                GROW_BLOCK(stmt->block, blk_capacity);
                            }
                            //printf("print_stmt() -> BLOCK scope %d stmt# %d\n", scope / 4, stmt->block->n_stmts);
                            INSERT_STMT(stmt->block, parse_stmt(toks, scope));

                        } else if (SPACE_INDENT(toks) < scope) {
                            break;
                        }
                    }
                }
            }
        }

        if (NOT_MAX_CAPACITY(stmt->block, blk_capacity)) {
            SHRINK_BLOCK(stmt->block);
        }

    } else if (NEXT_TOKEN(toks) == FUNCTION) { 
        //printf("parse_stmt() -> FUNCTION_DEF %s\n", toks->vec[toks->i + 1]->id);
        CONSUME_TOKEN(toks);  // expect 'fn'
        INIT_FUNCTION_DEF(stmt, toks);
        CONSUME_TOKEN(toks);  // expect '('

        while (NEXT_TOKEN(toks) != RIGHT_PAREN) {
            //printf("parse_stmt() -> FUNCTION_DEF -> PARAM #%d\n", stmt->defun->n_params);
            INSERT_PARAM(stmt->defun, parse_stmt(toks, 0));
            if (NEXT_TOKEN(toks) == COMMA) {
                CONSUME_TOKEN(toks); // expect ','
            }
        }
        CONSUME_TOKEN(toks); // expect ')'
        CONSUME_TOKEN(toks); // expect '->'
        INSERT_RETURN_TYPE(stmt->defun, CONSUME_TOKEN(toks));  // expect 'i32', 'i64', etc

        if (NOT_MAX_PARAMS(stmt->defun)) {
            SHRINK(stmt->defun);
        }
        INSERT_DEFINITION(stmt->defun, parse_stmt(toks, scope + 4));

    } else if (NEXT_TOKEN(toks) == PROCEDURE) {
        //printf("parse_stmt() -> PROCEDURE DECL\n");
        CONSUME_TOKEN(toks);   // expect 'proc'
        INIT_PROCEDURE_DEF(stmt, toks);
        CONSUME_TOKEN(toks); // expect '('

        while (NEXT_TOKEN(toks) != RIGHT_PAREN) {
            INSERT_PARAM(stmt->defproc, parse_stmt(toks, 0));
            if (NEXT_TOKEN(toks) == COMMA) {
                CONSUME_TOKEN(toks); // expect ','
            }
        }
        CONSUME_TOKEN(toks); // expect ')'

        if (NOT_MAX_PARAMS(stmt->defproc)) {
            SHRINK(stmt->defproc);
        }
        INSERT_DEFINITION(stmt->defproc, parse_stmt(toks, scope + 4));

    } else if (NEXT_TOKEN(toks) == IF) {
        //printf("parse_stmt() -> IF\n");
        CONSUME_TOKEN(toks);  // expect 'if'
        INIT_IF_STMT(stmt, toks, scope);
        int brnch_capacity = 2;

        while (NEXT_TOKEN(toks) == SPACES && SPACE_INDENT(toks) == scope 
                && (LOOK_AHEAD(toks) == ELSE || LOOK_AHEAD(toks) == ELIF)) {
            //printf("parse_if_body\n");
            CONSUME_TOKEN(toks); // expect ' '
            if (BRANCH_OVERFLOW(stmt->if_stmt, brnch_capacity)) {
                GROW_BRANCHES(stmt->if_stmt, brnch_capacity);
            }

            if (CONSUME_TOKEN(toks)->type == ELSE) {
                //printf("parse_stmt() -> ELSE\n");
                INIT_ELSE_STMT(stmt->if_stmt, toks, scope);
            } else {
                //printf("parse_stmt() -> ELIF\n");
                INIT_ELIF_STMT(stmt->if_stmt, toks, scope);
            }
        }

        if (NOT_MAX_BRANCHES(stmt->if_stmt, brnch_capacity)) {
            SHRINK_BRANCHES(stmt->if_stmt);
        }

    } else if (NEXT_TOKEN(toks) == WHILE) {
        //printf("parse_stmt() -> WHILE\n");
        CONSUME_TOKEN(toks); // expect 'while'
        INIT_WHILE_STMT(stmt, toks, scope);
    } else if (NEXT_TOKEN(toks) == STRUCT) {
        //printf("parse_stmt() -> STRUCT\n");
        CONSUME_TOKEN(toks);  // expect 'struct'
        INIT_STRUCT_DEF(stmt, toks); 
        CONSUME_TOKEN(toks);  // expect ':'
        CONSUME_TOKEN(toks);  // expect '\n'

        while (NOT_EOF(toks) && NEXT_TOKEN(toks) == SPACES) {
            if (LOOK_AHEAD(toks) == NEW_LINE) {
                SKIP_LINE(toks);
            } else {
                if (SPACE_INDENT(toks) == 4) {
                    CONSUME_TOKEN(toks);  // expect ' '
                    INSERT_FIELD(stmt->defstruct, parse_stmt(toks, 0));
                    CONSUME_TOKEN(toks);  // expect '\n'
                }
                // error -> indentation < 4 or > 4
            }
        }

        if (NOT_MAX_FIELD(stmt->defstruct)) {
            SHRINK_FIELD(stmt->defstruct);
        }
    }

    return stmt;
}


struct expr* parse_nud(struct tokens* toks, struct token* op) {
    struct expr* expr = malloc(sizeof(struct expr));

    if (IS_LITERAL(op)) {
        //printf("parse_nud() -> LITERAL\n");
        INIT_LITERAL_EXPR(expr, op);

    } else if (IS_UNARY(op)) {
        //printf("parse_nud() -> UNARY\n");
        INIT_UNARY_EXPR(expr, op, parse_expr(toks, 95));   // parse access-operations

    } else if (IS_GROUP(op)) {
        if (NEXT_IS_PRIMITIVE_TYPE(toks)) {   // EXPLICIT CONVERSION OPERATION
            //printf("parse_nud() -> CONVERSION\n");
            struct token* convert_type = CONSUME_TOKEN(toks);
            CONSUME_TOKEN(toks); // expect ')'
            INIT_UNARY_EXPR(expr, convert_type, parse_expr(toks, 95)); // parse access-operations

        } else {
            //printf("parse_nud() -> GROUP\n");
            expr = parse_expr(toks, 0);
            CONSUME_TOKEN(toks);   // expect ')'
        }

    } else if (IS_ALLOCATION(op)) {
        //printf("parse_nud() -> ALLOCATION\n");
        CONSUME_TOKEN(toks); // expect '('
        INIT_UNARY_EXPR(expr, op, parse_expr(toks, 0));
        CONSUME_TOKEN(toks); // expect ')'

    } else if (IS_IDENTIFIER(op)) {
        if (NEXT_TOKEN(toks) == LEFT_PAREN) {
            //printf("parse_nud() -> CALL\n");
            CONSUME_TOKEN(toks);      // expect '('
            INIT_CALL_EXPR(expr, op);

            while (NEXT_TOKEN(toks) != RIGHT_PAREN) {
                //printf("parse_nud() -> CALL arg# %d\n", expr->call->n_args);
                INSERT_ARG(expr->call, parse_expr(toks, 0));
                if (NEXT_TOKEN(toks) == COMMA) {
                    CONSUME_TOKEN(toks);  // expect ','
                }
            }
            CONSUME_TOKEN(toks); // expect ')'

            if (NOT_MAX_ARGS(expr->call)) {
                SHRINK_ARGS(expr->call);
            }

        } else {
            //printf("parse_nud() -> VARIABLE\n");
            INIT_VARIABLE_EXPR(expr, op);
        }
    }

    return expr;
}

struct expr* parse_led(struct expr* left, struct tokens* toks, struct token* op) {
    struct expr* expr = malloc(sizeof(struct expr)); 
    INIT_BINARY_EXPR(expr, left, op, parse_expr(toks, op->lbp));
    if (op->type == LEFT_BRACK) {
        CONSUME_TOKEN(toks); // expect ']'
    }
    return expr;
}

struct expr* parse_expr(struct tokens* toks, int rbp) {
    struct expr* left = parse_nud(toks, CONSUME_TOKEN(toks));

    while (!TERMINATING_SYMBOL(toks) && rbp < OP_BINDING_POWER(toks)) {
        left = parse_led(left, toks, CONSUME_TOKEN(toks));
    }
    return left;
}






