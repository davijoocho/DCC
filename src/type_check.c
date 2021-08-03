#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "type_check.h"

uint64_t compute_hash2(char* id) {
    uint64_t hash = 5381;
    for (int i = 0; id[i] != '\0'; i++) {
        hash = (hash * 33) ^ id[i]; 
    }
    return hash;
}

// GLOBAL SYMBOL TABLE
void construct_global_symtab(struct stmt** global_symtab, struct program* program) {
    memset(global_symtab, 0, sizeof(struct stmt*) * program->n_stmts * 2);

    for (int i = 0; i < program->n_stmts; i++) {
        struct stmt* entry = program->stmts[i];
        char* id = SYMBOL_ID(entry); 
        uint64_t idx = compute_hash2(id) % (program->n_stmts * 2);

        while (global_symtab[idx] != NULL)
            if (++idx == program->n_stmts * 2)
                idx = 0;

        global_symtab[idx] = entry;
    }
}


struct stmt* find_global_sym(struct stmt** global_symtab, char* id, int capacity) {
    uint64_t idx = compute_hash2(id) % capacity;

    while (global_symtab[idx] != NULL) {
        struct stmt* entry = global_symtab[idx];
        char* entry_id = SYMBOL_ID(entry); 

        if (!strcmp(id, entry_id))
            return global_symtab[idx];
        if (++idx == capacity)
            idx = 0;
    }
    return NULL;
}


// SCOPE STACK
void clean_up_scope(struct scope_info* scope, struct local_symtab* symtab) {
    while (scope->n_locals > 0) {
        struct locals* local = &scope->locals[scope->n_locals - 1];
        if (local->depth != scope->depth) {
            return;
        }
        delete_local_sym(symtab, local->id);
        scope->n_locals--;
    }
}

void push_local(struct scope_info* scope, char* id, int depth) {
    if (scope->n_locals == scope->capacity) {
        scope->locals = realloc(scope->locals, sizeof(struct locals) * scope->capacity * 2);
        scope->capacity *= 2;
    }
    struct locals* local = &scope->locals[scope->n_locals++];
    local->id = id;
    local->depth = depth;
}

// LOCAL SYMBOL TABLE
struct var_decl* find_local_sym(struct local_symtab* symtab, char* id) {
    uint64_t idx = compute_hash2(id) % symtab->capacity;

    while (symtab->decls[idx] != NULL) {
        if (!strcmp(symtab->decls[idx]->id->string, id))
            return symtab->decls[idx];
        if (++idx == symtab->capacity)
            idx = 0;
    }

    return NULL; 
}

void delete_local_sym(struct local_symtab* symtab, char* id) {
    uint64_t idx = compute_hash2(id) % symtab->capacity;
    struct var_decl* a = symtab->decls[idx];

    while (strcmp(symtab->decls[idx]->id->string, id) != 0)
        if (++idx == symtab->capacity)
            idx = 0;

    symtab->decls[idx] = NULL;
    symtab->n_decls--;
}


void grow_local_symtab(struct local_symtab* symtab, int new_capacity) {
    struct var_decl** new_decls = malloc(sizeof(struct stmt*) * new_capacity);
    memset(new_decls, 0, sizeof(struct stmt*) * new_capacity);

    for (int i = 0; i < symtab->capacity; i++) {
        struct var_decl* decl = symtab->decls[i];
        if (decl != NULL) {
            uint64_t new_idx = compute_hash2(decl->id->string) % new_capacity;
            while (new_decls[new_idx] != NULL)
                if (++new_idx == new_capacity)
                    new_idx = 0;

            new_decls[new_idx] = decl;
        }
    }
    symtab->decls = new_decls;
    symtab->capacity = new_capacity;
}


void insert_local(struct local_symtab* symtab, struct var_decl* var_decl) {
     if ((double)symtab->n_decls / symtab->capacity > 0.50) {
        grow_local_symtab(symtab, symtab->capacity * 2);
    }

    uint64_t idx = compute_hash2(var_decl->id->string) % symtab->capacity;
    while (symtab->decls[idx] != NULL) 
        if (++idx == symtab->capacity) 
            idx = 0;

    symtab->decls[idx] = var_decl;
    symtab->n_decls++;
}


// DRIVER 
struct expr* widen_to(enum token_type type, struct expr* eval) {
    struct expr* expr = malloc(sizeof(struct expr));
    struct unary* unary = malloc(sizeof(struct unary));
    struct token* token = malloc(sizeof(struct token));

    token->type = type;
    unary->op = token;
    unary->right = eval;
    expr->type = UNARY;
    expr->unary = unary;

    expr->eval_to = type;
    expr->indirect = 0;

    return expr;
}

struct expr* widen(struct stmt* stmt, struct expr* eval) {
    // just pass in type and indirect ?
    if (stmt->type == VAR_DECL_STMT) {
        struct var_decl* decl = stmt->var_decl;
        if ((decl->type->type != STRUCT_ID && decl->type->type != STRING && decl->indirect == 0) &&
                (eval->eval_to != STRUCT_ID && eval->eval_to != STRING && eval->eval_to != EMPTY && eval->indirect == 0)) {
            if (decl->type->type > eval->eval_to) {
                return widen_to(decl->type->type, eval);
            }
        }
    } else if (stmt->type == FUNCTION_DEF) {
        struct defun* fn = stmt->defun;
        if ((fn->return_type->type != STRUCT_ID && fn->return_type->type != STRING && fn->indirect == 0) &&
                (eval->eval_to != STRUCT_ID && eval->eval_to != STRING && eval->eval_to != EMPTY && eval->indirect == 0)) {
            if (fn->return_type->type > eval->eval_to) {
                return widen_to(fn->return_type->type, eval);
            }
        }
    } else if (stmt->type == ASSIGN_STMT) {
        struct assign* assign = stmt->assign;
        if ((assign->lhv->eval_to != STRUCT_ID && assign->lhv->eval_to != STRING && assign->lhv->indirect == 0) &&
                (eval->eval_to != STRUCT_ID && eval->eval_to != STRING && eval->eval_to != EMPTY && eval->indirect == 0)) {
            if (assign->lhv->eval_to > eval->eval_to) {
                return widen_to(assign->lhv->eval_to, eval);
            }
        }
    }
    return eval;
}

char* eval_to_repr(struct expr* expr) {
    char* type = malloc(64);
    int pos = 0;

    if (expr->eval_to == STRUCT_ID) {
        strcpy(type, expr->struct_id);
        pos = strlen(expr->struct_id);
    } else {
        char* types[] = {"c8", "i32", "i64", "f32", "f64", "string"};
        strcpy(type, types[expr->eval_to - C8]);
        pos = strlen(types[expr->eval_to - C8]);
    }

    for (int i = 0; i < expr->indirect; i++) {
        type[pos++] = '*';
    }
    type[pos] = '\0';

    return type;
}

enum error_code type_check_decl(struct stmt* stmt, struct expr* expr) {
    if (stmt->type == VAR_DECL_STMT) {
        if (expr->eval_to == EMPTY) {
            if (stmt->var_decl->type->type != STRING && stmt->var_decl->indirect == 0) {
                // string is essentialy a pointer.
                return INVALID_EMPTY;
            }
        } else {
            if (stmt->var_decl->indirect != expr->indirect || stmt->var_decl->type->type != expr->eval_to
                    || (expr->eval_to == STRUCT_ID && strcmp(expr->struct_id, stmt->var_decl->type->lexeme) != 0)) {
                return TYPE_INEQUALITY;
            }
        }
    } else if (stmt->type == FUNCTION_DEF) {
        if (expr->eval_to == EMPTY) {
            if (stmt->defun->return_type->type != STRING && stmt->defun->indirect == 0) {
                return INVALID_EMPTY;
            }
        } else {
            if (stmt->defun->indirect != expr->indirect || stmt->defun->return_type->type != expr->eval_to
                    || (expr->eval_to == STRUCT_ID && strcmp(expr->struct_id, stmt->defun->return_type->string) != 0)) {
                return TYPE_INEQUALITY;
            }
        }
    } else if (stmt->type == ASSIGN_STMT) {
        if (expr->eval_to == EMPTY) {
            if (stmt->assign->lhv->eval_to != STRING && stmt->assign->lhv->indirect == 0) {
                return INVALID_EMPTY;
            }
        } else {
            if (stmt->assign->lhv->indirect != expr->indirect || stmt->assign->lhv->eval_to != expr->eval_to
                    || (stmt->assign->lhv->eval_to == STRUCT_ID && strcmp(stmt->assign->lhv->struct_id, expr->struct_id) != 0)) {
                return TYPE_INEQUALITY;
            }
        }
    }
    return NO_ERROR;
}


void type_check_expr(struct local_symtab* local_symtab, struct stmt** global_symtab,
        struct expr* expr, struct program* program) {
    switch (expr->type) {
        case BINARY:
            // check which is bigger then widen1
            // widen is just adding type-cast
            // IGNORE DOT FOR NOW - NOT SUPPORTING STACK ALLOCATED STRUCTS
            // +, -, /, *, %, [], ->, <<, >>, |, &, or, and, is, isnt, <, >, <=, >=
            // accessing _FILE* is illegal
            // (string indirect 0) after access (char indirect 0)
            // eval_to->string must not also be STRING_LITERAL
            // pointer arithmetic
            // EMPTY SHOULDNT BE AN OPERAND
            // CHECK ERROR
            break;
        case UNARY:
            // ADDRESS-OF, LOGICAL_NOT, NEGATIVE, DEREFERENCE, TYPE_CAST
            break;
        case CALL:
            // OPEN, MALLOC, REALLOC are not in global_symtab, but we know these exists nonetheless
            // they need special support (parameter types, return type) 
            // eval_to = STRUCT_ID and then eval_to->struct_id = "_FILE"

            if (expr->call->id->type == MALLOC) {
            } else if (expr->call->id->type == REALLOC) {
            } else if (expr->call->id->type == OPEN) {
            } else {
                struct stmt* entry = find_global_sym(global_symtab, expr->call->id->string, program->n_stmts * 2);
                if (entry == NULL) {
                    printf("ERROR (LINE %d): function '%s' does not exist.\n", expr->call->id->line, expr->call->id->string);
                    local_symtab->error++;
                    expr->error = 1;
                } else {
                    struct defun* fn = entry->defun;

                    if (fn->n_params != expr->call->n_args) {
                        printf("ERROR (LINE %d): function '%s' requires %d argument(s).\n", expr->call->id->line, fn->id->string, fn->n_params); 
                        local_symtab->error++;
                    } else {
                        for (int i = 0; i < fn->n_params; i++) {
                            struct expr* arg = expr->call->args[i];
                            type_check_expr(local_symtab, global_symtab, arg, program);

                            if (!arg->error) {
                                expr->call->args[i] = widen(fn->params[i], arg);
                                enum error_code err = type_check_decl(fn->params[i], expr->call->args[i]);

                                if (err != NO_ERROR) {
                                    struct var_decl* param = fn->params[i]->var_decl;

                                    if (err == INVALID_EMPTY) {
                                        printf("ERROR (LINE %d): the parameter '%s' in function '%s' has type '%s' which is incompatible with 'Empty'.\n",
                                                expr->call->id->line, param->id->string, fn->id->string, param->type_repr);
                                    } else if (err == TYPE_INEQUALITY) {
                                        char* eval_to = eval_to_repr(arg);
                                        printf("ERROR (LINE %d): the parameter '%s' in function '%s has type '%s', but is passed an argument of type '%s'.\n", 
                                                expr->call->id->line, param->id->string, fn->id->string, param->type_repr, eval_to);
                                        free(eval_to);
                                    }
                                    local_symtab->error++;
                                }
                            }
                        }
                    }
                    expr->eval_to = fn->return_type->type;
                    expr->indirect = fn->indirect;
                    if (expr->eval_to == STRUCT_ID)
                        expr->struct_id = fn->return_type->string;
                }
            }
            break;
        case VARIABLE: {
                struct var_decl* decl = find_local_sym(local_symtab, expr->variable->id->string);
                if (decl == NULL) {
                    printf("ERROR (LINE %d): variable '%s' does not exist.\n", expr->variable->id->line, expr->variable->id->string);
                    local_symtab->error++;
                    expr->error = 1;
                } else {
                    expr->eval_to = decl->type->type;
                    expr->indirect = decl->indirect;
                    if (expr->eval_to == STRUCT_ID)
                        expr->struct_id = decl->type->string;
                }
            }
            break;
        case LITERAL:
            switch (expr->literal->value->type) {
                case CHARACTER: expr->eval_to = C8; break;
                case INTEGER: expr->eval_to = I32; break;
                case LONG: expr->eval_to = I64; break;
                case FLOAT: expr->eval_to = F32; break;
                case DOUBLE: expr->eval_to = F64; break;
                case EMPTY: expr->eval_to = EMPTY; break;
                case STRING_LITERAL: expr->eval_to = STRING; break;
                default: break;
            }
            break;
        default:
           break;
    }
}

void type_check_stmt(struct stmt* stmt, struct local_symtab* local_symtab, 
        struct stmt** global_symtab, struct scope_info* scope, struct program* program) {

    switch (stmt->type) {
        case PROCEDURE_DEF:
            for (int i = 0; i < stmt->defproc->n_params; i++) {
                push_local(scope, stmt->defproc->params[i]->var_decl->id->string, 1);
                insert_local(local_symtab, stmt->defproc->params[i]->var_decl);
            }
            type_check_stmt(stmt->defproc->def, local_symtab, global_symtab, scope, program);
            break;
        case FUNCTION_DEF: {
            struct defun* fn = stmt->defun;

            if (fn->return_type->type == STRUCT_ID) {
                if (find_global_sym(global_symtab, fn->return_type->lexeme, program->n_stmts * 2) == NULL) {
                    printf("ERROR (LINE %d): definition for struct '%s' does not exist. \n", fn->id->line, fn->return_type->lexeme);
                    local_symtab->error++;
                    return;
                }
            }

            for (int i = 0; i < fn->n_params; i++) {
                push_local(scope, fn->params[i]->var_decl->id->string, 1);
                insert_local(local_symtab, fn->params[i]->var_decl);
            }

            type_check_stmt(fn->def, local_symtab, global_symtab, scope, program);
            }
            break;
        case BLOCK_STMT:
            scope->depth++;
            for (int i = 0; i < stmt->block->n_stmts; i++) {
                type_check_stmt(stmt->block->stmts[i], local_symtab, global_symtab, scope, program);
            }
            clean_up_scope(scope, local_symtab);
            scope->depth--;
            break;

        // *****IF ERROR DETECTED IN TYPE_CHECK_EXPR EXIT OUT OF STATEMENT.
        // FOR VARIABLE, CALL_ID, IDENTIFIER NOT FOUND ISSUES
        case RETURN_STMT: 
            if (local_symtab->subrout->type == PROCEDURE_DEF) {
                if (stmt->ret->value != NULL) {
                    printf("ERROR (LINE %d): procedures cannot return values.\n", 
                            stmt->ret->token->line);
                    local_symtab->error++;
                }
            } else {
                type_check_expr(local_symtab, global_symtab, stmt->ret->value, program);
                if (!stmt->ret->value->error) {
                    stmt->ret->value = widen(local_symtab->subrout, stmt->ret->value);
                    enum error_code err = type_check_decl(local_symtab->subrout, stmt->ret->value);

                    if (err != NO_ERROR) {
                        struct defun* fn = local_symtab->subrout->defun;
                        local_symtab->error++;
                        if (err == INVALID_EMPTY) {
                            printf("ERROR (LINE %d): function '%s' has return type '%s', but is returning 'Empty' (only compatible with pointers).\n",
                                    stmt->ret->token->line, fn->id->string, fn->ret_type_repr);
                        } else if (err == TYPE_INEQUALITY) {
                            char* eval_to = eval_to_repr(stmt->ret->value);
                            printf("ERROR (LINE %d): function '%s' has return type '%s', but is returning an expression of type '%s'.\n", 
                                    stmt->ret->token->line, fn->id->string, fn->ret_type_repr, eval_to);
                            free(eval_to);
                        }
                    }
                }
            }
            
            break;

        case VAR_DECL_STMT: {
                struct var_decl* decl = find_local_sym(local_symtab, stmt->var_decl->id->string);
                if (decl != NULL) {
                    printf("ERROR (LINE %d): cannot use variable name '%s' because it already exists.\n", 
                            stmt->var_decl->id->line, stmt->var_decl->id->string);
                    local_symtab->error++;
                } else {
                    push_local(scope, stmt->var_decl->id->string, scope->depth);
                    insert_local(local_symtab, stmt->var_decl);

                    if (stmt->var_decl->array_literal) {
                        struct array_literal* arr = stmt->var_decl->value->array_literal;
                        for (int i = 0; i < arr->n_literals; i++) {
                            struct expr* expr = arr->literals[i];
                            type_check_expr(local_symtab, global_symtab, expr, program);   
                            // ASSUMPTION: TYPE IS NON-POINTER PRIMITIVE AND VALUES GUARANTEED TO BE LITERAL WITH CORRECT TYPE (NO WIDEN)

                            if (expr->eval_to != stmt->var_decl->type->type) {
                                char* l_type = eval_to_repr(expr);
                                printf("ERROR (LINE %d): value %s is type '%s', but the required type for elements in '%s' is '%s'\n",
                                        stmt->var_decl->id->line, expr->literal->value->lexeme, l_type, stmt->var_decl->id->string, stmt->var_decl->type_repr);
                                free(l_type);
                                local_symtab->error++;
                            }
                        }
                        return;
                    }


                    type_check_expr(local_symtab, global_symtab, stmt->var_decl->value, program);
                    if (!stmt->var_decl->value->error) {
                        stmt->var_decl->value = widen(stmt, stmt->var_decl->value);
                        enum error_code err = type_check_decl(stmt, stmt->var_decl->value);

                        if (err != NO_ERROR) {
                            if (err == INVALID_EMPTY) {
                                printf("ERROR (LINE %d): Cannot assign 'Empty' to variable '%s' with non-pointer type '%s'\n", 
                                        stmt->var_decl->id->line, stmt->var_decl->id->string, stmt->var_decl->type_repr);
                            } else if (err == TYPE_INEQUALITY) {
                                char* expr_repr = eval_to_repr(stmt->var_decl->value);
                                printf("ERROR (LINE %d): Cannot assign expression of type '%s' to variable '%s' with type '%s'\n", 
                                        stmt->var_decl->id->line, expr_repr, stmt->var_decl->id->string, stmt->var_decl->type_repr);
                                free(expr_repr);
                            }
                            local_symtab->error++;
                        }
                    }
                }
            }
            break;

        case ASSIGN_STMT:
            type_check_expr(local_symtab, global_symtab, stmt->assign->lhv, program);
            if (!stmt->assign->lhv->error) {
                if (stmt->assign->lhv->type == VARIABLE) {
                    struct var_decl* decl = find_local_sym(local_symtab, stmt->assign->lhv->variable->id->string);
                    if (decl->array_literal) {
                        printf("ERROR (LINE %d): Cannot reassign values to arrays or structs allocated on the stack.\n", 
                                stmt->assign->lhv->variable->id->line);
                        local_symtab->error++;
                        return;
                    }
                }

                type_check_expr(local_symtab, global_symtab, stmt->assign->rhv, program);
                if  (!stmt->assign->rhv->error) {
                    stmt->assign->rhv = widen(stmt, stmt->assign->rhv);
                    enum error_code err = type_check_decl(stmt, stmt->assign->rhv);
                    if (err != NO_ERROR) {
                        if (err == INVALID_EMPTY) {
                            char* l_value = eval_to_repr(stmt->assign->lhv);
                            printf("ERROR (LINE %d): Cannot assign 'Empty' to l-value of non-pointer type '%s'\n", 
                                    stmt->assign->lhv->line, l_value);
                            free(l_value);
                        } else if (err == TYPE_INEQUALITY) {
                            char* l_value = eval_to_repr(stmt->assign->lhv);
                            char* r_value = eval_to_repr(stmt->assign->rhv);
                            printf("ERROR (LINE %d): Cannot assign r-value of type '%s' to l-value of type '%s'\n", 
                                    stmt->assign->lhv->line, r_value, l_value);
                            free(l_value);
                            free(r_value);
                        }
                        local_symtab->error++;
                    }
                }
            }
            break;
        case PROCEDURAL_CALL:
            // CLOSE, READ, WRITE, MEMCPY, PRINT, FREE are not in global_symtab, but we know these exists nonetheless
            // they need special support (parameter types) 
            // procedure exists
            break;

        case IF_STMT:
            type_check_expr(local_symtab, global_symtab, stmt->if_stmt->cond, program);
            type_check_stmt(stmt->if_stmt->body, local_symtab, global_symtab, scope, program);
            for (int i = 0; i < stmt->if_stmt->n_elifs; i++) {
                type_check_expr(local_symtab, global_symtab, stmt->if_stmt->elifs[i]->cond, program);
                type_check_stmt(stmt->if_stmt->elifs[i]->body, local_symtab, global_symtab, scope, program);
            }
            if (stmt->if_stmt->_else != NULL) {
                type_check_stmt(stmt->if_stmt->_else->body, local_symtab, global_symtab, scope, program);
            }
            break;
        case WHILE_STMT:
            type_check_expr(local_symtab, global_symtab, stmt->while_stmt->cond, program);
            type_check_stmt(stmt->while_stmt->body, local_symtab, global_symtab, scope, program);
            break;
        default:
            break;
    }
}

void semantic_analysis(struct program* program) {
    struct stmt** global_symtab = malloc(sizeof(struct stmt*) * program->n_stmts * 2);
    construct_global_symtab(global_symtab, program);

    struct local_symtab* local_symtab = malloc(sizeof(struct local_symtab));
    local_symtab->decls = malloc(sizeof(struct var_decl*) * 32);

    local_symtab->n_decls = 0;
    local_symtab->capacity = 32;
    local_symtab->error = 0;

    struct scope_info* scope = malloc(sizeof(struct scope_info));
    scope->locals = malloc(sizeof(struct locals) * 16);

    scope->n_locals = 0;
    scope->capacity = 16;
    scope->depth = 0;

    int main = 0;

    for (int i = 0; i < program->n_stmts; i++) {
        if (program->stmts[i]->type != STRUCT_DEF) {  // i.e FUNCTION_DEF, PROCEDURE_DEF
            if (program->stmts[i]->type == PROCEDURE_DEF) {
                if (program->stmts[i]->defproc->id->type == MAIN) {
                    main = 1;
                }
            }
            local_symtab->subrout = program->stmts[i];
            type_check_stmt(program->stmts[i], local_symtab, global_symtab, scope, program);
        }
    }


    if (!main) {
        printf("ERROR: could not find 'main' procedure.\n");
        local_symtab->error++;
    }

    if (local_symtab->error) {
        exit(0);
    }

    free(global_symtab);
    free(local_symtab->decls);
    free(local_symtab);
    free(scope->locals);
    free(scope);
}

