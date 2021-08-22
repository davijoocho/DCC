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
    expr->line = eval->line;
    expr->indirect = 0;

    return expr;
}

struct expr* widen(enum token_type l_eval_to, int l_indirect, struct expr* eval) {
    if ((l_eval_to != STRUCT_ID && l_eval_to != STRING && l_indirect == 0) 
            && (eval->eval_to != STRUCT_ID && eval->eval_to != STRING && eval->eval_to != EMPTY && eval->indirect == 0)) {
        if (l_eval_to > eval->eval_to) {
            return widen_to(l_eval_to, eval);
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
    } else if (expr->eval_to == EMPTY) {
        strcpy(type, "Empty");
        pos = 5;
    } else if (expr->eval_to == STRING && expr->type == LITERAL) {
        strcpy(type, "lstring");
        pos = 7;
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

enum error_code type_check_decl(enum token_type l_eval_to, int l_indirect, char* l_struct_id, struct expr* expr) {
    if (expr->eval_to == EMPTY) {
        if (l_eval_to != STRING && l_indirect == 0) {
            return INVALID_EMPTY;
        }
    } else {
        if (l_indirect != expr->indirect || l_eval_to != expr->eval_to 
                || (expr->eval_to == STRUCT_ID && strcmp(expr->struct_id, l_struct_id) != 0)) {
            return TYPE_INEQUALITY;
        }
    }

    return NO_ERROR;
}

void print_expr_error(struct expr* expr, struct expr* left, struct expr* right) {
    char* l_type = eval_to_repr(left);
    char* r_type = eval_to_repr(right);
    printf(ANSI_COLOR_RED "ERROR" 
            ANSI_COLOR_RESET " (LINE %d): invalid operands to binary operator '%s' ('%s' and '%s').\n", 
            expr->line, expr->binary->op->lexeme, l_type, r_type);
    free(l_type);
    free(r_type);
}

void print_unary_error(struct expr* expr, struct expr* right) {
    char* r_type = eval_to_repr(right);
    printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): invalid operand to unary operator '%s' (%s).\n",
            expr->line, expr->unary->op->lexeme, r_type);
    free(r_type);
}

void implicit_conversion(struct expr* op, struct expr* left, struct expr* right) {
    if (left->eval_to < right->eval_to) {
        op->binary->left = widen_to(right->eval_to, left);
    } else {
        op->binary->right = widen_to(left->eval_to, right);
    }
}

int non_access_pre_check(struct expr* expr, struct expr* left, struct expr* right, struct local_symtab* local_symtab) {
    if (left->eval_to == EMPTY || right->eval_to == EMPTY) {
        enum token_type op = expr->binary->op->type;
        if (op != IS && op != ISNT && op != LT && op != LTEQ && op != GT && op != GTEQ) {
            printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): 'Empty' cannot be an operand in an arithmetic operation.\n",
                    expr->line);
            expr->error = 1;
            local_symtab->error++;
            return 1;
        }
    }

    if ((left->type == CALL && 
                (left->call->id->type == MALLOC || left->call->id->type == REALLOC || left->call->id->type == OPEN))
            || (right->type == CALL && (right->call->id->type == MALLOC || right->call->id->type == REALLOC || right->call->id->type == OPEN))) {
        printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): calls to standard library functions cannot be an operand in an expression.\n",
                expr->line);
        expr->error = 1;
        local_symtab->error++;
        return 1;
    }

    if ((left->eval_to == STRING && left->type == LITERAL)
            || (right->eval_to == STRING && right->type == LITERAL)) {
        printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): string literal cannot be an operand in an expression.\n",
                expr->line);
        expr->error = 1;
        local_symtab->error++;
        return 1;
    }

    if (left->error || right->error) {
        expr->error = 1;
        return 1;
    }

    return 0;
}


void type_check_expr(struct local_symtab* local_symtab, struct stmt** global_symtab,
        struct expr* expr, struct program* program) {
    switch (expr->type) {
        case BINARY: {
            struct expr* left = expr->binary->left;
            struct expr* right = expr->binary->right;

            type_check_expr(local_symtab, global_symtab, left, program);

            switch (expr->binary->op->type) {
                case PLUS:
                case MINUS:
                    type_check_expr(local_symtab, global_symtab, right, program);
                    if (!non_access_pre_check(expr, left, right, local_symtab)) {
                        if (left->indirect == 0 && right->indirect == 0 
                                && left->eval_to != STRING && right->eval_to != STRING) {
                            if (left->eval_to != STRUCT_ID && right->eval_to != STRUCT_ID) {
                                // PRIMITIVE
                                if (left->eval_to != right->eval_to) {
                                    implicit_conversion(expr, left, right);
                                }
                                expr->eval_to = right->eval_to;
                            } else {
                                print_expr_error(expr, left, right);
                                expr->error = 1;
                                local_symtab->error++;
                            }
                        } else {
                            if (right->indirect > 0 || right->eval_to == STRING) {
                                if (left->indirect == 0 && (left->eval_to == C8 || left->eval_to == I32 || left->eval_to == I64)) {
                                    // POINTER ARITHMETIC (INTEGRAL)
                                    expr->binary->left = widen(I64, 0, left);
                                    expr->eval_to = right->eval_to;
                                    expr->indirect = right->indirect;
                                } else {
                                    print_expr_error(expr, left, right);
                                    expr->error = 1;
                                    local_symtab->error++;
                                } 
                            } else {
                                if ((left->indirect > 0 || left->eval_to == STRING) 
                                        && (right->eval_to == C8 || right->eval_to == I32 || right->eval_to == I64)) {
                                    // POINTER ARITHMETIC (INTEGRAL)
                                    expr->binary->right = widen(I64, 0, right);
                                    expr->eval_to = left->eval_to;
                                    expr->indirect = left->indirect;
                                } else {
                                    print_expr_error(expr, left, right);
                                    expr->error = 1;
                                    local_symtab->error++;
                                }
                            }
                        }
                    }
                        
                    break;

                case DIVIDE:
                case STAR:
                    type_check_expr(local_symtab, global_symtab, right, program);
                    if (!non_access_pre_check(expr, left, right, local_symtab)) {
                        if ((left->eval_to != STRING && left->eval_to != STRING) 
                                && (right->eval_to != STRUCT_ID && right->eval_to != STRING) 
                                && (left->indirect == 0 && right->indirect == 0)) {
                            // ARITHMETIC (PRIMITIVE) 
                            if (left->eval_to != right->eval_to) {
                                implicit_conversion(expr, left, right);
                            }
                            expr->eval_to = right->eval_to;
                        } else {
                            print_expr_error(expr, left, right);
                            expr->error = 1;
                            local_symtab->error++;
                        }
                    }
                    break;

                case MODULO:
                case BIT_OR:
                case AND:
                    type_check_expr(local_symtab, global_symtab, right, program);
                    if (!non_access_pre_check(expr, left, right, local_symtab)) {
                        if ((left->eval_to == I32 || left->eval_to == I64) 
                                && (right->eval_to == I32 || right->eval_to == I64)
                                && (left->indirect == 0 && right->indirect == 0)) {
                            // ARITHMETIC (PRIMITIVE)
                            if (left->eval_to != right->eval_to) {
                                implicit_conversion(expr, left, right);
                            }
                            expr->eval_to = right->eval_to;
                        } else {
                            print_expr_error(expr, left, right);
                            expr->error = 1;
                            local_symtab->error++;
                        }
                    }
                    break;

                case BIT_SHIFTR:
                case BIT_SHIFTL:
                    type_check_expr(local_symtab, global_symtab, right, program);
                    if (!non_access_pre_check(expr, left, right, local_symtab)) {
                        if ((left->eval_to == C8 || left->eval_to == I32 || left->eval_to == I64)
                                && (right->eval_to == C8 || right->eval_to == I32 || right->eval_to == I64)) {
                            // ARITHMETIC (PRIMITIVE)
                            expr->eval_to = left->eval_to;
                        } else {
                            print_expr_error(expr, left, right);
                            expr->error = 1;
                            local_symtab->error++;
                        }
                    }
                    break;

                case LOGICAL_OR:
                case LOGICAL_AND:
                    type_check_expr(local_symtab, global_symtab, right, program);
                    if (!non_access_pre_check(expr, left, right, local_symtab)) {
                        if ((left->eval_to == STRUCT_ID && left->indirect == 0)
                                || (right->eval_to == STRUCT_ID && right->indirect == 0)) {
                            print_expr_error(expr, left, right);
                            expr->error = 1;
                            local_symtab->error++;
                        } else {
                            // NO STACK-ALLOCATED STRUCTS
                            expr->eval_to = I32;
                        }
                    }
                    break;

                case IS:
                case ISNT:
                case LT:
                case LTEQ:
                case GT:
                case GTEQ:
                    type_check_expr(local_symtab, global_symtab, right, program);
                    if (!non_access_pre_check(expr, left, right, local_symtab)) {
                        if (left->indirect == 0 && right->indirect == 0 
                                && left->eval_to != STRING && right->eval_to != STRING) {
                            if (left->eval_to == STRUCT_ID || right->eval_to == STRUCT_ID) {
                                print_expr_error(expr, left, right);
                                expr->error = 1;
                                local_symtab->error++;
                            } else {
                                if (left->eval_to == EMPTY || right->eval_to == EMPTY) {
                                    if (left->eval_to == EMPTY && right->eval_to != EMPTY && right->eval_to != I64) {
                                        expr->binary->right = widen(I64, 0, right);
                                    } else if (left->eval_to != EMPTY && right->eval_to == EMPTY && left->eval_to != I64) {
                                        expr->binary->left = widen(I64, 0, left);
                                    }
                                } else {
                                    if (left->eval_to != right->eval_to) {
                                        implicit_conversion(expr, left, right);
                                    }
                                }
                            }
                        } else {
                            if (right->indirect > 0 || right->eval_to == STRING) {
                                if (left->indirect == 0 && left->eval_to != STRING) {
                                    if (left->eval_to == STRUCT_ID) {
                                        print_expr_error(expr, left, right);
                                        expr->error = 1;
                                        local_symtab->error++;
                                    } else {
                                        if (left->eval_to != I64 && left->eval_to != EMPTY) {
                                            expr->binary->left = widen(I64, 0, left);
                                        }
                                    }
                                }
                            } else {
                                if (left->indirect > 0 || left->eval_to == STRING) {
                                    if (right->indirect == 0 && right->eval_to != STRING) {
                                        if (right->eval_to == STRUCT_ID) {
                                            print_expr_error(expr, left, right);
                                            expr->error = 1;
                                            local_symtab->error++;
                                        } else {
                                            if (right->eval_to != I64 && right->eval_to != EMPTY) {
                                                expr->binary->right = widen(I64, 0, right);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                        
                    expr->eval_to = I32;
                    break;

                case LEFT_BRACK:
                    type_check_expr(local_symtab, global_symtab, right, program);
                    if (!non_access_pre_check(expr, left, right, local_symtab)) {
                        if (right->eval_to == I32 || right->eval_to == I64) {
                            if (left->indirect > 0) {
                                expr->eval_to = left->eval_to;
                                expr->indirect = left->indirect - 1;
                                expr->struct_id = left->struct_id;
                            } else {
                                if (left->eval_to == STRING) {
                                    expr->eval_to = C8;
                                    expr->indirect = 0;
                                } else {
                                    print_expr_error(expr, left, right);
                                    expr->error = 1;
                                    local_symtab->error++;
                                }
                            }

                        } else {
                            print_expr_error(expr, left, right);
                            expr->error = 1;
                            local_symtab->error++;
                        }
                    }
                    break;

                case ARROW:

                    if (left->error) {
                        expr->error = 1;
                        return;
                    }

                    if (left->eval_to == STRUCT_ID && left->indirect == 1) {
                        if (right->type == VARIABLE) {
                            struct defstruct* _struct = find_global_sym(global_symtab, left->struct_id, program->n_stmts * 2)->defstruct;

                            for (int i = 0; i < _struct->n_fields; i++) {
                                struct var_decl* field = _struct->fields[i]->var_decl;
                                if (strcmp(field->id->string, right->variable->id->string) == 0) {
                                    expr->eval_to = field->type->type;
                                    expr->indirect = field->indirect;
                                    if (expr->eval_to == STRUCT_ID)
                                        expr->struct_id = field->type->string;
                                    return;
                                }
                            }

                            printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): field '%s' does not exist in struct '%s'.\n",
                                    left->line, right->variable->id->string, left->struct_id);
                            expr->error = 1;
                            local_symtab->error++;
                        } 
                    } else {
                        char* l_type = eval_to_repr(left);
                        printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): invalid operand to operator '->' (%s).\n", 
                                left->line, l_type);
                        free(l_type);
                        expr->error = 1;
                        local_symtab->error++;
                    }
                    break;

                default:
                    break;
            }
        }

            break;
        case UNARY: {
            struct expr* right = expr->unary->right;
            type_check_expr(local_symtab, global_symtab, right, program);

            if (right->error) {
                expr->error = 1;
                return;
            }

            if (right->type == CALL && (right->call->id->type == MALLOC || right->call->id->type == REALLOC || right->call->id->type == OPEN)) {
                printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): calls to standard library functions cannot be an operand in an expression.\n",
                        expr->line);
                expr->error = 1;
                local_symtab->error++;
                return;
            }


            if ((expr->unary->op->type != LOGICAL_NOT && right->eval_to == EMPTY) || 
                    (right->eval_to == STRING && right->type == LITERAL)) {
                print_unary_error(expr, right);
                expr->error = 1;
                local_symtab->error++;
                return;
            }


            switch (expr->unary->op->type) {
                case C8:
                case I32:
                case I64:
                case F32:
                case F64:
                    if (right->eval_to == STRUCT_ID && right->indirect == 0) {
                        print_unary_error(expr, right);
                        expr->error = 1;
                        local_symtab->error++;
                        return;
                    }
                    expr->eval_to = expr->unary->op->type;
                    break;

                case AND:
                    if (right->type == VARIABLE ||
                            (right->type == BINARY &&
                             (right->binary->op->type == LEFT_BRACK || right->binary->op->type == ARROW))) {
                        expr->eval_to = right->eval_to;
                        expr->indirect = right->indirect + 1;
                        expr->struct_id = right->struct_id;
                    } else {
                        print_unary_error(expr, right);
                        expr->error = 1;
                        local_symtab->error++;
                    }

                    break;

                case STAR:
                    if (right->indirect > 0) {
                        expr->eval_to = right->eval_to;
                        expr->indirect = right->indirect - 1;
                        expr->struct_id = right->struct_id;
                    } else {
                        if (right->eval_to == STRING) {
                            expr->eval_to = C8;
                            expr->indirect = 0;
                        } else {
                            print_unary_error(expr, right);
                            expr->error = 1;
                            local_symtab->error++;
                        }
                    }
                    break;

                case MINUS:
                    if (right->indirect > 0 || right->eval_to == STRING 
                            || right->eval_to == STRUCT_ID) {
                        print_unary_error(expr, right);
                        expr->error = 1;
                        local_symtab->error++;
                        return;
                    }

                    expr->eval_to = right->eval_to;
                    break;

                case LOGICAL_NOT:
                    if (right->eval_to == STRUCT_ID && right->indirect == 0) {
                        print_unary_error(expr, right);
                        expr->error = 1;
                        local_symtab->error++;
                        return;
                    }
                    expr->eval_to = I32;
                    break;
                default:
                    break;
            }

            }
            break;

        case CALL: {
            // MALLOC, REALLOC, OPEN
            // they need special support (parameter types, return type) 
            // eval_to = STRUCT_ID and then eval_to->struct_id = "_FILE"

            struct stmt* entry = find_global_sym(global_symtab, expr->call->id->string, program->n_stmts * 2);
            if (entry == NULL) {
                printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): function '%s' does not exist.\n", expr->call->id->line, expr->call->id->string);
                local_symtab->error++;
                expr->error = 1;
            } else {
                struct defun* fn = entry->defun;

                if (fn->n_params != expr->call->n_args) {
                    printf(ANSI_COLOR_RED "ERROR" 
                            ANSI_COLOR_RESET " (LINE %d): function '%s' requires %d argument(s).\n", expr->call->id->line, fn->id->string, fn->n_params); 
                    local_symtab->error++;
                } else {
                    for (int i = 0; i < fn->n_params; i++) {
                        type_check_expr(local_symtab, global_symtab, expr->call->args[i], program);

                        if (!expr->call->args[i]->error) {
                            struct var_decl* param = fn->params[i]->var_decl;

                            // check if param->eval_to == VOID* (reserved for standard library functions)
                            if (param->type->type == VOID && param->indirect == 1) {
                                if (expr->call->args[i]->indirect == 0) {
                                    char* eval_to = eval_to_repr(expr->call->args[i]);
                                    printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): the parameter '%s' in function '%s' has type 'void*', but is passed an argument of type '%s'.\n", 
                                            expr->line, param->id->string, fn->id->string, eval_to);
                                    free(eval_to);
                                    local_symtab->error++;
                                }
                                break;
                            }

                            expr->call->args[i] = widen(param->type->type, param->indirect, expr->call->args[i]);
                            enum error_code err = type_check_decl(param->type->type, param->indirect, param->type->string, expr->call->args[i]);

                            if (err != NO_ERROR) {
                                if (err == INVALID_EMPTY) {
                                    printf(ANSI_COLOR_RED "ERROR" 
                                            ANSI_COLOR_RESET " (LINE %d): the parameter '%s' in function '%s' has type '%s' which is incompatible with 'Empty'.\n",
                                            expr->call->id->line, param->id->string, fn->id->string, param->type_repr);
                                } else if (err == TYPE_INEQUALITY) {
                                    char* eval_to = eval_to_repr(expr->call->args[i]);
                                    printf(ANSI_COLOR_RED "ERROR" 
                                            ANSI_COLOR_RESET " (LINE %d): the parameter '%s' in function '%s has type '%s', but is passed an argument of type '%s'.\n", 
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
                    printf(ANSI_COLOR_RED "ERROR" 
                            ANSI_COLOR_RESET " (LINE %d): variable '%s' does not exist.\n", expr->variable->id->line, expr->variable->id->string);
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
                    printf(ANSI_COLOR_RED "ERROR" 
                            ANSI_COLOR_RESET " (LINE %d): definition for struct '%s' does not exist. \n", fn->id->line, fn->return_type->lexeme);
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

        case RETURN_STMT: 
            if (local_symtab->subrout->type == PROCEDURE_DEF) {
                if (stmt->ret->value != NULL) {
                    printf(ANSI_COLOR_RED "ERROR"
                            ANSI_COLOR_RESET " (LINE %d): procedures cannot return values.\n", 
                            stmt->ret->token->line);
                    local_symtab->error++;
                }
            } else {
                type_check_expr(local_symtab, global_symtab, stmt->ret->value, program);

                // check if returning call here too standard library
                if (!stmt->ret->value->error) {
                    struct defun* fn = local_symtab->subrout->defun;
                    stmt->ret->value = widen(fn->return_type->type, fn->indirect, stmt->ret->value);
                    enum error_code err = type_check_decl(fn->return_type->type, fn->indirect, fn->return_type->string, stmt->ret->value);

                    if (err != NO_ERROR) {
                        local_symtab->error++;
                        if (err == INVALID_EMPTY) {
                            printf(ANSI_COLOR_RED "ERROR" 
                                    ANSI_COLOR_RESET " (LINE %d): function '%s' has return type '%s', but is returning 'Empty' (only compatible with pointers).\n",
                                    stmt->ret->token->line, fn->id->string, fn->ret_type_repr);
                        } else if (err == TYPE_INEQUALITY) {
                            char* eval_to = eval_to_repr(stmt->ret->value);
                            printf(ANSI_COLOR_RED "ERROR" 
                                    ANSI_COLOR_RESET " (LINE %d): function '%s' has return type '%s', but is returning an expression of type '%s'.\n", 
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
                    printf(ANSI_COLOR_RED "ERROR" 
                            ANSI_COLOR_RESET " (LINE %d): cannot use variable name '%s' because it is already declared.\n", 
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
                                printf(ANSI_COLOR_RED "ERROR" 
                                        ANSI_COLOR_RESET " (LINE %d): value %s is type '%s', but the required type for elements in '%s' is '%s'\n",
                                        stmt->var_decl->id->line, expr->literal->value->lexeme, l_type, stmt->var_decl->id->string, stmt->var_decl->type_repr);
                                free(l_type);
                                local_symtab->error++;
                            }
                        }
                        return;
                    }


                    type_check_expr(local_symtab, global_symtab, stmt->var_decl->value, program);

                    // fix after implementing void* type
                    if (stmt->var_decl->value->type == CALL) {
                        struct call* call = stmt->var_decl->value->call;
                       if (call->id->type == MALLOC || call->id->type == REALLOC) {
                           if (stmt->var_decl->indirect == 0) {
                               printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): Cannot assign expression of type 'void*' to variable '%s' with type '%s'\n",
                                       stmt->var_decl->id->line, stmt->var_decl->id->string, stmt->var_decl->type_repr);
                               local_symtab->error++;
                           }
                           return;
                       }
                    }

                    if (!stmt->var_decl->value->error) {
                        stmt->var_decl->value = widen(stmt->var_decl->type->type, stmt->var_decl->indirect, stmt->var_decl->value);
                        enum error_code err = type_check_decl(stmt->var_decl->type->type, stmt->var_decl->indirect,
                                stmt->var_decl->type->string, stmt->var_decl->value);

                        if (err != NO_ERROR) {
                            if (err == INVALID_EMPTY) {
                                printf(ANSI_COLOR_RED "ERROR" 
                                        ANSI_COLOR_RESET " (LINE %d): Cannot assign 'Empty' to variable '%s' with non-pointer type '%s'\n", 
                                        stmt->var_decl->id->line, stmt->var_decl->id->string, stmt->var_decl->type_repr);
                            } else if (err == TYPE_INEQUALITY) {
                                char* expr_repr = eval_to_repr(stmt->var_decl->value);
                                printf(ANSI_COLOR_RED "ERROR" 
                                        ANSI_COLOR_RESET " (LINE %d): Cannot assign expression of type '%s' to variable '%s' with type '%s'\n", 
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
                        printf(ANSI_COLOR_RED "ERROR"
                                ANSI_COLOR_RESET " (LINE %d): Cannot reassign values to arrays or structs allocated on the stack.\n", 
                                stmt->assign->lhv->variable->id->line);
                        local_symtab->error++;
                        return;
                    }
                }

                if (stmt->assign->lhv->type == BINARY) {
                    struct binary* bi = stmt->assign->lhv->binary;
                    if (bi->op->type == LEFT_BRACK) {
                        if (bi->left->eval_to == STRING && bi->left->indirect == 0) {
                            printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): Mutating strings is not allowed.\n", stmt->assign->lhv->line);
                            local_symtab->error++;
                            return;
                        }
                    }
                } 

                type_check_expr(local_symtab, global_symtab, stmt->assign->rhv, program);

                // fix after implementing void* type
                if (stmt->assign->rhv->type == CALL) {
                    struct call* call = stmt->assign->rhv->call;
                    if (call->id->type == MALLOC || call->id->type == REALLOC) {
                        if (stmt->assign->lhv->indirect == 0) {
                            char* l_value = eval_to_repr(stmt->assign->lhv);
                            printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): Cannot assign r-value of type 'void*' to l-value of type '%s'\n",
                                   stmt->assign->lhv->line, l_value);
                            free(l_value);
                            local_symtab->error++;
                        }
                        return;
                    }
                }

                if  (!stmt->assign->rhv->error) {
                    stmt->assign->rhv = widen(stmt->assign->lhv->eval_to, stmt->assign->lhv->indirect, stmt->assign->rhv);
                    enum error_code err = type_check_decl(stmt->assign->lhv->eval_to, stmt->assign->lhv->indirect,
                            stmt->assign->lhv->struct_id, stmt->assign->rhv);
                    if (err != NO_ERROR) {
                        if (err == INVALID_EMPTY) {
                            char* l_value = eval_to_repr(stmt->assign->lhv);
                            printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): Cannot assign 'Empty' to l-value of non-pointer type '%s'\n", 
                                    stmt->assign->lhv->line, l_value);
                            free(l_value);
                        } else if (err == TYPE_INEQUALITY) {
                            char* l_value = eval_to_repr(stmt->assign->lhv);
                            char* r_value = eval_to_repr(stmt->assign->rhv);
                            printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): Cannot assign r-value of type '%s' to l-value of type '%s'\n", 
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
            if (stmt->proc->id->type == PRINT) {
                // check that arg #1 is string literal
                char* format = stmt->proc->args[0]->literal->value->string;
                int c = 0;
                int idx = 1;

                while (format[c] != '\0') {
                    if (format[c++] == '%') {
                        if (idx == stmt->proc->n_args) {
                            printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): number of '%%' conversions does not match arguments to printf.\n", stmt->proc->id->line);
                            local_symtab->error++;
                            return;
                        }

                        enum token_type expected_type = 0;
                        int expected_indirect = 0;

                        struct expr* arg = stmt->proc->args[idx];
                        type_check_expr(local_symtab, global_symtab, arg, program);

                        if (format[c] == 'd') {
                            expected_type = I32;
                        } else if (format[c] == 'l') {
                            expected_type = I64;
                        } else if (format[c] == 'f') {
                            expected_type = F64;
                        } else if (format[c] == 's') {
                            expected_type = C8;
                            expected_indirect = 1;
                        }

                        stmt->proc->args[idx] = widen(expected_type, expected_indirect, arg);
                        enum error_code err = type_check_decl(expected_type, expected_indirect, NULL, stmt->proc->args[idx]);

                        if (err != NO_ERROR) {
                            if (err == TYPE_INEQUALITY) {
                                char* eval_to = eval_to_repr(stmt->proc->args[idx]);
                                printf(ANSI_COLOR_RED "ERROR" 
                                        ANSI_COLOR_RESET " (LINE %d): conversion #%d in printf is passed an argument with incorrect type '%s'.\n", 
                                        stmt->proc->id->line, idx, eval_to);
                                free(eval_to);
                            }
                        }

                        idx++;
                        c++;
                    }
                }

            } else {
                struct stmt* entry = find_global_sym(global_symtab, stmt->proc->id->string, program->n_stmts * 2);
                if (entry == NULL) {
                    printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): procedure '%s' does not exist.\n", stmt->proc->id->line, stmt->proc->id->string);
                    local_symtab->error++;
                } else {
                    struct defproc* defproc = entry->defproc;
                    if (defproc->n_params != stmt->proc->n_args) {
                        printf(ANSI_COLOR_RED "ERROR" 
                                ANSI_COLOR_RESET " (LINE %d): procedure '%s' requires %d argument(s).\n", 
                                stmt->proc->id->line, stmt->proc->id->string, defproc->n_params); 
                        local_symtab->error++;
                    } else {
                        for (int i = 0; i < stmt->proc->n_args; i++) {
                            type_check_expr(local_symtab, global_symtab, stmt->proc->args[i], program);

                            if (!stmt->proc->args[i]->error) {
                                struct var_decl* param = defproc->params[i]->var_decl;

                                // check if param->eval_to == VOID* (reserved for standard library functions)
                                if (param->type->type == VOID && param->indirect == 1) {
                                    if (stmt->proc->args[i]->indirect == 0) {
                                        char* eval_to = eval_to_repr(stmt->proc->args[i]);
                                        printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): the parameter '%s' in procedure '%s' has type 'void*', but is passed an argument of type '%s'.\n", 
                                                stmt->proc->id->line, param->id->string, stmt->proc->id->string, eval_to);
                                        free(eval_to);
                                        local_symtab->error++;
                                    }
                                    break;
                                }

                                stmt->proc->args[i] = widen(param->type->type, param->indirect, stmt->proc->args[i]);
                                enum error_code err = type_check_decl(param->type->type, param->indirect, param->type->string, stmt->proc->args[i]);

                                if (err != NO_ERROR) {
                                    if (err == INVALID_EMPTY) {
                                        printf(ANSI_COLOR_RED "ERROR" 
                                                ANSI_COLOR_RESET " (LINE %d): the parameter '%s' in procedure '%s' has type '%s' which is incompatible with 'Empty'.\n",
                                                stmt->proc->id->line, param->id->string, defproc->id->string, param->type_repr);
                                    } else if (err == TYPE_INEQUALITY) {
                                        char* eval_to = eval_to_repr(stmt->proc->args[i]);
                                        printf(ANSI_COLOR_RED "ERROR" 
                                                ANSI_COLOR_RESET " (LINE %d): the parameter '%s' in procedure '%s has type '%s', but is passed an argument of type '%s'.\n", 
                                                stmt->proc->id->line, param->id->string, defproc->id->string, param->type_repr, eval_to);
                                        free(eval_to);
                                    }
                                    local_symtab->error++;
                                }
                            }
                        }
                    }
                }
            }
            break;

        case IF_STMT:
            type_check_expr(local_symtab, global_symtab, stmt->if_stmt->cond, program);
            if (!stmt->if_stmt->cond->error) {
                if (stmt->if_stmt->cond->eval_to == STRUCT_ID && stmt->if_stmt->cond->indirect == 0) {
                    printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): invalid conditional expression.\n", stmt->if_stmt->cond->line);
                }
            }

            type_check_stmt(stmt->if_stmt->body, local_symtab, global_symtab, scope, program);
            for (int i = 0; i < stmt->if_stmt->n_elifs; i++) {
                type_check_expr(local_symtab, global_symtab, stmt->if_stmt->elifs[i]->cond, program);
                if (stmt->if_stmt->elifs[i]->cond->eval_to == STRUCT_ID && stmt->if_stmt->elifs[i]->cond->indirect == 0) {
                    printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): invalid conditional expression.\n", stmt->if_stmt->elifs[i]->cond->line);
                }
                type_check_stmt(stmt->if_stmt->elifs[i]->body, local_symtab, global_symtab, scope, program);
            }
            if (stmt->if_stmt->_else != NULL) {
                type_check_stmt(stmt->if_stmt->_else->body, local_symtab, global_symtab, scope, program);
            }
            break;
        case WHILE_STMT:
            type_check_expr(local_symtab, global_symtab, stmt->while_stmt->cond, program);
            if (stmt->while_stmt->cond->eval_to == STRUCT_ID && stmt->while_stmt->cond->indirect == 0) {
                printf(ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET " (LINE %d): invalid conditional expression.\n", stmt->while_stmt->cond->line);
            }
            type_check_stmt(stmt->while_stmt->body, local_symtab, global_symtab, scope, program);
            break;
        default:
            break;
    }
}



struct stmt** semantic_analysis(struct program* program) {
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
            if (program->stmts[i]->type == FUNCTION_DEF) {
                if (program->stmts[i]->defun->id->type == MAIN) {
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

    free(local_symtab->decls);
    free(local_symtab);
    free(scope->locals);
    free(scope);

    return global_symtab;
}

