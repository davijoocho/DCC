#include <stdlib.h>
#include <stdio.h>
#include "parser.h"

struct program* syntax_analysis(struct tokens* tokens) {
    struct program* program = malloc(sizeof(struct program));
    int capacity = 256;
    program->stmts = malloc(sizeof(struct stmt*) * capacity);
    program->n_stmts = 0;

    while (tokens->tokens[tokens->idx]->type != EOFF) {
        if (program->n_stmts == capacity) {
            program->stmts = realloc(program->stmts, sizeof(struct stmt*) * capacity * 2);
            capacity *= 2;
        }
        //printf("syntax_analysis()\n");
        program->stmts[program->n_stmts++] = parse_stmt(tokens, 0);
    }


    if (program->n_stmts < capacity) {
        program->stmts = realloc(program->stmts, sizeof(struct stmt*) * program->n_stmts);
    }

    free(tokens);
    return program;
}


struct stmt* parse_stmt(struct tokens* tokens, int scope) {
    struct stmt* stmt = malloc(sizeof(struct stmt));

    switch (tokens->tokens[tokens->idx]->type) {
        case READ:
        case CLOSE:
        case PRINT:
        case FREE:
        case MEMCPY:
        case WRITE:
        case IDENTIFIER:   // PROCEDURE
            if (tokens->tokens[tokens->idx + 1]->type == LEFT_PAREN) {
                //printf("parse_stmt() - PROCEDURE\n");
                struct proc* proc = malloc(sizeof(struct proc));
                proc->id = tokens->tokens[tokens->idx++]; 
                proc->args = malloc(sizeof(struct expr*) * 6);  // 6 max arguments for bootstrap compiler
                proc->n_args = 0;

                tokens->idx++; // expect '('

                while (tokens->tokens[tokens->idx]->type != RIGHT_PAREN) {
                    proc->args[proc->n_args++] = parse_expr(tokens, 0);
                    if (tokens->tokens[tokens->idx]->type == COMMA) {
                        tokens->idx++;  // expect ','
                    }
                }

                tokens->idx++; // expect ')'
                tokens->idx++; // expect '\n'

                stmt->type = PROCEDURAL_CALL;
                stmt->proc = proc;
            } else {  // ASSIGN
                //printf("parse_stmt() - ASSIGN\n");
                struct assign* assign = malloc(sizeof(struct assign));
                assign->lhv = parse_expr(tokens, 0);
                tokens->idx++;  // expect '='
                assign->rhv = parse_expr(tokens, 0);

                tokens->idx++; // expect '\n'

                stmt->assign = assign;
                stmt->type = ASSIGN_STMT;
            }
            break;

        case C8:
        case I32:
        case I64:
        case F32:
        case F64:
        case STRUCT_ID:
        case _FILE:
        case STRING: {   // DECLARATION
            //printf("parse_stmt() - DECLARATION\n");
            struct var_decl* var_decl = malloc(sizeof(struct var_decl));
            var_decl->type = tokens->tokens[tokens->idx++];
            var_decl->indirect = 0;
            var_decl->array_literal = 0;

            while (tokens->tokens[tokens->idx]->type == STAR) {
                var_decl->indirect++;
                tokens->idx++;
            }

            var_decl->id = tokens->tokens[tokens->idx++];

            if (scope == 0) {   
                // PARAMETER
                var_decl->value = NULL;
            } else {            
                // LOCAL_VARIABLE
                if (tokens->tokens[tokens->idx]->type == LEFT_BRACK) {
                    // not allowed to declare stack-allocated array in parameter
                    var_decl->array_literal = 1;
                    var_decl->indirect++;
                    tokens->idx += 2; // expect '[' and ']'
                }
                tokens->idx++; // expect '='
                var_decl->value = parse_expr(tokens, 0);
                tokens->idx++; // expect '\n'
            }

            stmt->var_decl = var_decl;
            stmt->type = VAR_DECL_STMT;
            }
            break;

        case RETURN: { // RETURN
            //printf("parse_stmt() - RETURN\n");
            tokens->idx++; // expect 'ret'
            struct ret* ret = malloc(sizeof(struct ret));
            ret->value = NULL; // PROCEDURES CAN USE RETURN TO TERMINATE FUNCTION

            if (tokens->tokens[tokens->idx]->type != NEW_LINE)
                ret->value = parse_expr(tokens, 0);

            tokens->idx++; // expect '\n'

            stmt->ret = ret;
            stmt->type = RETURN_STMT;
            }
            break;

        case COLON: {  // SCOPE
            //printf("parse_stmt() - SCOPE %d\n", scope / 4);
            struct block* block = malloc(sizeof(struct block));
            int blk_capacity = 128;
            block->stmts = malloc(sizeof(struct stmt*) * blk_capacity);
            block->n_stmts = 0;

            tokens->idx++;  // expect ':'
            tokens->idx++; // expect '\n'

            struct token* nxt = tokens->tokens[tokens->idx];
            while (nxt->type == INDENT && nxt->i32 == scope) {
                // CHECK_ERROR: indent is higher, indent is not divisible by 4
                tokens->idx++; // expect IDENT
                if (block->n_stmts == blk_capacity) {
                    block->stmts = realloc(block->stmts, sizeof(struct stmt*) * blk_capacity * 2);
                    blk_capacity *= 2;
                }

                //printf("parse_stmt() - STATEMENT\n");
                block->stmts[block->n_stmts++] = parse_stmt(tokens, scope);
                nxt = tokens->tokens[tokens->idx];
            }

            if (block->n_stmts < blk_capacity) {
                block->stmts = realloc(block->stmts, sizeof(struct stmt*) * block->n_stmts);
            }

            stmt->block = block;
            stmt->type = BLOCK_STMT;
            }
            break;

        case FUNCTION: {
            //printf("parse_stmt() - FUNCTION\n");
            tokens->idx++; // expect 'fn'
            struct defun* fn = malloc(sizeof(struct defun));
            fn->id = tokens->tokens[tokens->idx++]; 
            fn->params = malloc(sizeof(struct stmt*) * 6);
            fn->n_params = 0;

            tokens->idx++;  // expect '('

            while (tokens->tokens[tokens->idx]->type != RIGHT_PAREN) {
                fn->params[fn->n_params++] = parse_stmt(tokens, 0);
                if (tokens->tokens[tokens->idx]->type == COMMA) {
                    tokens->idx++; // expect ','
                }
            }

            tokens->idx++; // expect ')'
            tokens->idx++; // expect '->'

            fn->return_type = tokens->tokens[tokens->idx++]; // expect 'i32', 'i64', etc
            fn->indirect = 0;

            while (tokens->tokens[tokens->idx]->type == STAR) {
                fn->indirect++;
                tokens->idx++;
            }

            fn->def = parse_stmt(tokens, 4);

            stmt->defun = fn;
            stmt->type = FUNCTION_DEF;
            }
            break;


        case PROCEDURE: {
            //printf("parse_stmt() - PROCEDURE\n");
            tokens->idx++; // expect 'proc'
            struct defproc* proc = malloc(sizeof(struct defproc));
            proc->id = tokens->tokens[tokens->idx++];
            proc->params = malloc(sizeof(struct stmt*) * 6);
            proc->n_params = 0;

            tokens->idx++; // expect '('
            while (tokens->tokens[tokens->idx]->type != RIGHT_PAREN) {
                proc->params[proc->n_params++] = parse_stmt(tokens, 0);
                if (tokens->tokens[tokens->idx]->type == COMMA) {
                    tokens->idx++; // expect ','
                }
            }

            tokens->idx++; // expect ')'
            proc->def = parse_stmt(tokens, 4);

            stmt->defproc = proc;
            stmt->type = PROCEDURE_DEF;
            }
            break;

        case WHILE: {
            //printf("parse_stmt() - WHILE\n");
            tokens->idx++; // expect 'while'
            struct while_stmt* _while = malloc(sizeof(struct while_stmt));
            _while->cond = parse_expr(tokens, 0);
            _while->body = parse_stmt(tokens, scope + 4);

            stmt->while_stmt = _while;
            stmt->type = WHILE_STMT;
            }
            break;

        case IF: {
            //printf("parse_stmt() - IF\n");
            tokens->idx++; // expect 'if'

            struct if_stmt* _if = malloc(sizeof(struct if_stmt));
            _if->cond = parse_expr(tokens, 0);
            _if->body = parse_stmt(tokens, scope + 4);

            int capacity = 2;
            _if->elifs = malloc(sizeof(struct elif_stmt*) * capacity);
            _if->n_elifs = 0;
            _if->_else = NULL; 

            struct token* nxt = tokens->tokens[tokens->idx];
            struct token* look_ahead = tokens->tokens[tokens->idx + 1];

            while (nxt->type == INDENT && nxt->i32 == scope &&
                    (look_ahead->type == ELIF || look_ahead->type == ELSE)) {
                tokens->idx += 2; // expect INDENT and (ELSE or ELIF)

                if (look_ahead->type == ELIF) {
                    if (_if->n_elifs == capacity) {
                        _if->elifs = realloc(_if->elifs, sizeof(struct elif_stmt*) * capacity * 2);
                        capacity *= 2;
                    }
                    //printf("parse_stmt() - ELIF\n");
                    struct elif_stmt* elif = malloc(sizeof(struct elif_stmt));
                    elif->cond = parse_expr(tokens, 0);
                    elif->body = parse_stmt(tokens, scope + 4);
                    _if->elifs[_if->n_elifs++] = elif;
                } else {
                    //printf("parse_stmt() - ELSE\n");
                    struct else_stmt* _else = malloc(sizeof(struct else_stmt));
                    _else->body = parse_stmt(tokens, scope + 4);
                    _if->_else = _else;
                }

                nxt = tokens->tokens[tokens->idx];
                look_ahead = tokens->tokens[tokens->idx + 1];
            }

            if (_if->n_elifs < capacity) {
                _if->elifs = realloc(_if->elifs, sizeof(struct elif_stmt*) * _if->n_elifs);
            }

            stmt->if_stmt = _if;
            stmt->type = IF_STMT;
            }
            break;

        case STRUCT: {
            //printf("parse_stmt() - STRUCT\n");
            tokens->idx++; // expect 'struct'
            struct defstruct* _struct = malloc(sizeof(struct defstruct));
            _struct->id = tokens->tokens[tokens->idx++];
            _struct->fields = malloc(sizeof(struct stmt*) * 16);  // 16 max fields for bootstrap compiler
            _struct->n_fields = 0;

            tokens->idx++; // expect ':'
            tokens->idx++; // expect '\n'

            while (tokens->tokens[tokens->idx]->type == INDENT) {
                // CHECK_ERROR: indentation is not 4
                //printf("parse_stmt() - FIELD\n");
                tokens->idx++; // expect INDENT
                _struct->fields[_struct->n_fields++] = parse_stmt(tokens, 0);
                tokens->idx++; // expect '\n'
            }

            if (_struct->n_fields < 16) {
                _struct->fields = realloc(_struct->fields, sizeof(struct stmt*) * _struct->n_fields);
            }

            stmt->defstruct = _struct;
            stmt->type = STRUCT_DEF;
            }
            break;
        default:
            break;
    }

    return stmt;
}

struct expr* parse_nud(struct tokens* tokens, struct token* op) {
    struct expr* expr = malloc(sizeof(struct expr));
    expr->indirect = 0;   //  FOR EXPLICIT TYPE-CAST
    expr->struct_id = NULL;

    switch (op->type) {
        case INTEGER:
        case LONG:
        case FLOAT:
        case DOUBLE:
        case CHARACTER:
        case STRING_LITERAL:
        case EMPTY: {
            //printf("parse_nud() - LITERAL\n");
            struct literal* literal = malloc(sizeof(struct literal));
            literal->value = op;
            expr->literal = literal;
            expr->type = LITERAL;
            }
            break;

        case STAR: // DEREFERENCE
        case AND:  // ADDRESS-OF
        case LOGICAL_NOT:
        case MINUS: {  // NEGATIVE
            //printf("parse_nud() - UNARY\n");
            struct unary* unary = malloc(sizeof(struct unary));
            unary->op = op;
            unary->right = parse_expr(tokens, 99);
            expr->unary = unary;
            expr->type = UNARY;
            }
            break;

        case LEFT_PAREN: {
            struct token* nxt = tokens->tokens[tokens->idx++];
            if (nxt->type == I32 || nxt->type == I64 || nxt->type == F32 || nxt->type == F64 ||
                    nxt->type == C8 || nxt->type == STRING || nxt->type == STRUCT_ID) {  // EXPLICIT TYPE-CAST (PRIMITIVE)

                //printf("parse_nud() - TYPE_CAST\n");
                struct unary* type_cast = malloc(sizeof(struct unary));
                type_cast->op = nxt;

                while (tokens->tokens[tokens->idx]->type == STAR) {  
                    expr->indirect++;
                    tokens->idx++;  // expect '*'
                }

                tokens->idx++;  // expect ')'

                type_cast->right = parse_expr(tokens, 99);
                expr->unary = type_cast;
                expr->type = UNARY;
            } else {  // GROUPING
                //printf("parse_nud() - GROUPING\n");
                free(expr);
                expr = parse_expr(tokens, 0);
                tokens->idx++; // expect ')'
            }

            }
            break;

        case OPEN:
        case MALLOC:
        case REALLOC:
        case IDENTIFIER:
            if (tokens->tokens[tokens->idx]->type == LEFT_PAREN) {
                //printf("parse_nud() - CALL\n");
                tokens->idx++; // expect '('
                struct call* call = malloc(sizeof(struct call));
                call->id = op;
                call->args = malloc(sizeof(struct expr*) * 6);
                call->n_args = 0;

                while (tokens->tokens[tokens->idx]->type != RIGHT_PAREN) {
                    //printf("parse_nud() - ARG\n");
                    call->args[call->n_args++] = parse_expr(tokens, 0);
                    if (tokens->tokens[tokens->idx]->type == COMMA) {
                        tokens->idx++; // expect ','
                    }
                }

                tokens->idx++; // expect ')'

                expr->call = call;
                expr->type = CALL;
            } else {
                //printf("parse_nud() - VARIABLE\n");
                struct variable* variable = malloc(sizeof(struct variable));
                variable->id = op;
                expr->variable = variable;
                expr->type = VARIABLE;
            }
            break;


        case LEFT_BRACE: {   // ARRAY LITERAL 
            //printf("parse_nud() - ARRAY_LITERAL\n");
            struct array_literal* arr = malloc(sizeof(struct array_literal));
            int capacity = 16;
            arr->literals = malloc(sizeof(struct expr*) * capacity);
            arr->n_literals = 0;

            while (tokens->tokens[tokens->idx]->type != RIGHT_BRACE) {
                if (arr->n_literals == capacity) {
                    arr->literals = realloc(arr->literals, sizeof(struct expr*) * capacity * 2);
                    capacity *= 2;
                }
                // array literal in bootstrap version of compiler forbids expressions, therefore no parse_expr (requires literals)
                // assumption: parse_nud returns literal
                arr->literals[arr->n_literals++] = parse_nud(tokens, tokens->tokens[tokens->idx++]); 
            }

            tokens->idx++; // expect '}'

            if (arr->n_literals < capacity) {
                arr->literals = realloc(arr->literals, sizeof(struct expr*) * arr->n_literals);
            }

            expr->array_literal = arr;
            expr->type = ARRAY_LITERAL;
            }
            break;

        default:
            break;
    }
    return expr;
}



struct expr* parse_led(struct expr* left, struct tokens* tokens, struct token* op) {
    //printf("parse_led() %d\n", op->type);
    struct expr* expr = malloc(sizeof(struct expr)); 

    struct binary* binary = malloc(sizeof(struct binary));
    binary->op = op;
    binary->left = left;
    binary->right = parse_expr(tokens, op->lbp);

    if (op->type == LEFT_BRACK) {
        tokens->idx++;  // expect ']'
    }

    expr->binary = binary;
    expr->type = BINARY;

    return expr;
}

struct expr* parse_expr(struct tokens* tokens, int rbp) {
    //printf("parse_expr()\n");
    struct expr* left = parse_nud(tokens, tokens->tokens[tokens->idx++]);

    struct token* nxt = tokens->tokens[tokens->idx];
    while (!(nxt->type == COMMA || nxt->type == NEW_LINE || nxt->type == RIGHT_PAREN ||
                nxt->type == COLON || nxt->type == ASSIGN || nxt->type == RIGHT_BRACK)   
            // no RIGHT_BRACE for bootstrap because array literal requires literals (i.e no expressions)
            && rbp < nxt->lbp) {
        left = parse_led(left, tokens, tokens->tokens[tokens->idx++]);
        nxt = tokens->tokens[tokens->idx];
    }

    return left;
}




