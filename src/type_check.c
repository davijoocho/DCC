#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "type_check.h"


// utility
uint64_t compute_hash(char* id) {
    uint64_t hash = 5381;
    for (int i = 0; id[i] != '\0'; i++) {
        hash = (hash * 33) ^ id[i]; 
    }
    return hash;
}

// global
void insert_global_sym(struct stmt** global_symtab, struct stmt* sym, uint64_t idx, int max) {
    while (global_symtab[idx] != NULL) {
        // error -> conflicting ids
        if (++idx == max)
            idx = 0;
    }
    global_symtab[idx] = sym;
}

void build_global_symtab(struct stmt** global_symtab, struct program* program) {
    for (int i = 0; i < program->n; i++) {
        int idx = compute_hash(GET_SYMBOL_ID(program->stmts[i])) % (program->n * 2);
        insert_global_sym(global_symtab, program->stmts[i], idx, program->n * 2);
    }
}

struct stmt* find_global_sym(struct stmt** symtab, char* id, int capacity) {
    uint64_t idx = compute_hash(id) % capacity;

    while (symtab[idx] != NULL) {
        if (strcmp(DECL_ID(symtab[idx]), id) == 0) {
            return symtab[idx];
        } else {
            if (++idx == capacity) {
                idx = 0;
            }
        }
    }

    return NULL; 
}

// local
struct stmt* find_local_sym(struct local_symtab* symtab, char* id) {
    uint64_t idx = compute_hash(id) % symtab->capacity;

    while (symtab->decls[idx] != NULL) {
        if (strcmp(DECL_ID(symtab->decls[idx]), id) == 0) {
            return symtab->decls[idx];
        } else {
            if (++idx == symtab->capacity) {
                idx = 0;
            }
        }
    }

    return NULL; 
}

void delete_local_sym(struct local_symtab* symtab, char* id) {
    uint64_t idx = compute_hash(id) % symtab->capacity;

    while (strcmp(DECL_ID(symtab->decls[idx]), id) != 0) {    // guaranteed to be in symbol table
        if (++idx == symtab->capacity) {
            idx = 0;
        }
    }

    symtab->decls[idx] = NULL;
    symtab->n_decls--;
}

void grow_local_symtab(struct local_symtab* symtab, int new_capacity) {
    struct stmt** new_decls = malloc(sizeof(struct stmt*) * new_capacity);

    for (int i = 0; i < symtab->capacity; i++) {
        uint64_t new_idx = compute_hash(DECL_ID(symtab->decls[i])) % new_capacity;
        while (new_decls[new_idx] != NULL) {
            if (++new_idx == new_capacity) {
                new_idx = 0;
            }
        }
        new_decls[new_idx] = symtab->decls[i];
    }
    symtab->decls = new_decls;
    symtab->capacity = new_capacity;
}

void insert_local(struct local_symtab* symtab, struct stmt* decl) {
    if ((double)symtab->n_decls / symtab->capacity > 0.75) {
        grow_local_symtab(symtab, symtab->capacity * 2);
    }
    uint64_t idx = compute_hash(DECL_ID(decl)) % symtab->capacity;
    while (symtab->decls[idx] != NULL) {
        if (++idx == symtab->capacity) {
            idx = 0;
        }
    }
    symtab->decls[idx] = decl;
}


// scope
void clean_up_scope(struct scope* scope, struct local_symtab* symtab) {
    while (scope->n_locals > 0) {
        struct locals* local = &scope->locals[scope->n_locals - 1];
        if (local->depth != scope->depth) {
            return;
        }
        delete_local_sym(symtab, local->id);
        scope->n_locals--;
    }
}

void push_local(struct scope* scope, char* id, int depth) {
    if (scope->n_locals == scope->capacity) {
        scope->locals = realloc(scope->locals, sizeof(struct locals) * scope->capacity * 2);
        scope->capacity *= 2;
    }
    struct locals* local = &scope->locals[scope->n_locals++];
    local->id = id;
    local->depth = depth;
}

// expressions

struct expr* widen(struct expr* expr_to_widen, enum token_type widen_to) {
    struct expr* conversion = malloc(sizeof(struct expr));
    struct token* op_token = malloc(sizeof(struct token));

    conversion->eval_to = widen_to;
    conversion->indirect = 0;
    op_token->type = widen_to;

    INIT_UNARY_EXPR(conversion, op_token, expr_to_widen);

    return conversion;
}

void type_check_operands(struct expr* expr) {
    if (NOT_EQUAL_TYPES(BIN_LEFT(expr), BIN_RIGHT(expr))) {
        if (BIN_LEFT(expr)->eval_to < BIN_RIGHT(expr)->eval_to) {
            expr->binary->left = widen(BIN_LEFT(expr), BIN_RIGHT(expr)->eval_to);
        } else {
            expr->binary->right = widen(BIN_RIGHT(expr), BIN_LEFT(expr)->eval_to);
        }
        expr->indirect = 0;
    }
}

void type_check_expr(struct local_symtab* local_symtab, struct stmt** global_symtab, 
        struct expr* expr, struct program* program) {
    switch (expr->type) {
        case BINARY:
            type_check_expr(local_symtab, global_symtab, BIN_LEFT(expr), program);

            if (ARITHMETIC(expr)) {
                type_check_expr(local_symtab, global_symtab, BIN_RIGHT(expr), program);
                if (VALID_ARITHMETIC_TYPE(BIN_LEFT(expr)) && VALID_ARITHMETIC_TYPE(BIN_RIGHT(expr))) {
                    type_check_operands(expr);
                    expr->eval_to = BIN_LEFT(expr)->eval_to;   // abritrary 
                } else {  // invalid types (arithmetic: +, -, *, /)
                    printf("COMPILATION ERROR (LINE %d): invalid operand types.", expr->binary->op->line);
                    exit(0);
                }

            } else if (REQUIRES_INTEGRAL(expr)) {
                type_check_expr(local_symtab, global_symtab, BIN_RIGHT(expr), program);
                if (VALID_INTEGRAL_TYPE(BIN_LEFT(expr)) && VALID_INTEGRAL_TYPE(BIN_RIGHT(expr))) {
                    type_check_operands(expr);
                    expr->eval_to = BIN_LEFT(expr)->eval_to;    // arbritrary
                } else {  // invalid types (integral: <<, >>, |, &, %)
                    printf("COMPILATION ERROR (LINE %d): invalid operand types.", expr->binary->op->line);
                    exit(0);
                }

            } else if (COMPARISON(expr)) {
                type_check_expr(local_symtab, global_symtab, BIN_RIGHT(expr), program);
                if (VALID_ARITHMETIC_TYPE(BIN_LEFT(expr)) && VALID_ARITHMETIC_TYPE(BIN_RIGHT(expr))) {
                    type_check_operands(expr);
                    expr->eval_to = BOOL; 
                } else {  // invalid types (comparison: <, >, <=, >=)
                    printf("COMPILATION ERROR (LINE %d): invalid operand types.", expr->binary->op->line);
                    exit(0);
                }

            } else if (CONNECTIVE(expr)) {
                type_check_expr(local_symtab, global_symtab, BIN_RIGHT(expr), program);
                if (BOOLEAN(BIN_LEFT(expr)) && BOOLEAN(BIN_RIGHT(expr))) {
                    expr->eval_to = BOOL; 
                } else {  // invalid types (connective: and or)
                    printf("COMPILATION ERROR (LINE %d): invalid operand types.", expr->binary->op->line);
                    exit(0);
                }

            } else if (EQUALITY(expr)) {
                type_check_expr(local_symtab, global_symtab, BIN_RIGHT(expr), program);
                if (BOOLEAN(BIN_LEFT(expr)) && BOOLEAN(BIN_RIGHT(expr))) {
                    expr->eval_to = BOOL;
                } else if (VALID_ARITHMETIC_TYPE(BIN_LEFT(expr)) && VALID_ARITHMETIC_TYPE(BIN_RIGHT(expr))) {
                    type_check_operands(expr);
                    expr->eval_to = BOOL;
                } else {  // invalid types (equality: is isnt)
                    printf("COMPILATION ERROR (LINE %d): invalid operand types.", expr->binary->op->line);
                    exit(0);
                }
        
            } else if (ARRAY_INDEX(expr)) {
                type_check_expr(local_symtab, global_symtab, BIN_RIGHT(expr), program);
                if (ARRAY(BIN_LEFT(expr))) {
                    if (VALID_INTEGRAL_TYPE(BIN_RIGHT(expr))) {
                        expr->eval_to = BIN_LEFT(expr)->eval_to;
                        expr->indirect = BIN_LEFT(expr)->indirect - 1;
                        expr->struct_id = BIN_LEFT(expr)->struct_id;
                    } else {
                        printf("COMPILATION ERROR (LINE %d): index is not of type integral.", expr->binary->op->line);
                        exit(0);
                    }
                } else {
                    printf("COMPILATION ERROR (LINE %d): attempting to access index of non-array type.", expr->binary->op->line);
                    exit(0);
                }

            } else if (FIELD_ACCESS(expr)) {
                if (VALID_STRUCT(BIN_LEFT(expr))) {
                    if (IDENTIFIER(BIN_RIGHT(expr))) {
                        struct stmt* defstruct = find_global_sym(global_symtab, BIN_LEFT(expr)->struct_id, program->n * 2);

                        for (int i = 0; i < N_FIELDS(defstruct); i++) {
                            struct stmt* field = GET_FIELD(defstruct, i);
                            if (strcmp(DECL_ID(field), VAR_ID(BIN_RIGHT(expr)))) {
                                expr->eval_to = DECL_TYPE(field);
                                expr->indirect = DECL_INDIRECT(field);
                                if (expr->eval_to == STRUCT_ID) {
                                    expr->struct_id = DECL_TYPE_ID(field);
                                }
                                return;
                            }
                        }

                        printf("COMPILATION ERROR (LINE %d): field %s does not exist in struct %s.", 
                                expr->binary->op->line, VAR_ID(BIN_RIGHT(expr)), BIN_LEFT(expr)->struct_id);
                        exit(0);

                    } else {
                        printf("COMPILATION ERROR (LINE %d): attempting to access struct with invalid field operand.", expr->binary->op->line);
                        exit(0);
                    }
                } else {
                    printf("COMPILATION ERROR (LINE %d): attempting to access field of non-struct type.", expr->binary->op->line);
                    exit(0);
                }
            }
            break;

        case UNARY:
            // CONVERSION
            // BIT_NOT
            // NOT
            // MINUS
            // ALLOCATE
        case CALL:
            break;



        case VARIABLE: {
            struct stmt* decl = find_local_sym(local_symtab, VAR_ID(expr));
            if (decl == NULL) {  // error -> undeclared variable
                printf("ERROR (LINE %d): variable %s is used before declaration.", expr->variable->id->line, VAR_ID(expr)); 
                exit(0);
            }

            expr->eval_to = DECL_TYPE(decl);
            if (expr->eval_to == STRUCT_ID) {
                expr->struct_id = DECL_TYPE_ID(decl);
            }
            expr->indirect = DECL_INDIRECT(decl);
        }
            break;
        case LITERAL:
            switch (LITERAL_TYPE(expr)) {
                case CHARACTER: expr->eval_to = C8; break;
                case INTEGER: expr->eval_to = I32; break;
                case LONG: expr->eval_to = I64; break;
                case FLOAT: expr->eval_to = F32; break;
                case DOUBLE: expr->eval_to = F64; break;
                case TRUE: 
                case FALSE: 
                    expr->eval_to = BOOL;
                    break;
                default: break; // unreachable
            }
            break;
        default:
            break;
    }
}

// driver

void type_check_stmt(struct stmt* stmt, struct local_symtab* local_symtab, 
        struct stmt** global_symtab, struct scope* scope, struct program* program) {
    switch (stmt->type) {
        case PROCEDURE_DEF:
        case FUNCTION_DEF:
            for (int i = 0; i < N_PARAMS(stmt); i++) {
                push_local(scope, ID(stmt), 1);
                insert_local(local_symtab, PARAM_DECL(stmt, i));
            }
            type_check_stmt(DEFINITION(stmt), local_symtab, global_symtab, scope, program);
            break;

        case BLOCK_STMT:
            ENTER_SCOPE(scope);
            for (int i = 0; i < BLOCK(stmt)->n_stmts; i++) {
                type_check_stmt(BLOCK(stmt)->stmts[i], local_symtab, global_symtab, scope, program);
            }
            clean_up_scope(scope, local_symtab);
            EXIT_SCOPE(scope);
            break;

        case RETURN_STMT:
            break;

        case VAR_DECL_STMT:
            type_check_expr(local_symtab, global_symtab, stmt->var_decl->value, program);
            // perform check
            // on right_hand side make sure struct definition exists globally
            push_local(scope, DECL_ID(stmt), scope->depth);
            insert_local(local_symtab, stmt);
            break;

        case ASSIGN_STMT:
            break;
        case PROCEDURAL_CALL:
            break;
        case IF_STMT:
            break;
        case WHILE_STMT:
            break;
        default:
            break;
    }
    // FIND_LOCAL_VAR
    // return match to local_symtab->func->ret_type
    // local_symtab->func->type = PROCEDURE then return->value should be NULL
}

void type_check(struct program* program) {
    struct stmt** global_symtab;
    BUILD_GLOBAL_SYMTAB(global_symtab, program);
    for (int i = 0; i < program->n; i++) {
        if (program->stmts[i]->type == FUNCTION_DEF ||
                program->stmts[i]->type == PROCEDURE_DEF) {
            struct local_symtab local_symtab;
            INIT_LOCAL_SYMTAB(local_symtab, program->stmts[i]);
            struct scope scope;
            INIT_SCOPE(scope);

            type_check_stmt(program->stmts[i], &local_symtab, global_symtab, &scope, program);
            FREE(local_symtab, scope);
        }
    }
}
