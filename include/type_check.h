#ifndef TYPE_CHECK_H
#define TYPE_CHECK_H

#include <stddef.h>
#include "parser.h"
// TYPE_CHECK_STMT()

// FUNCTION/ PROCEDURE
#define N_PARAMS(stmt) (stmt->type == PROCEDURE_DEF) ? stmt->defproc->n_params : stmt->defun->n_params
#define ID(stmt) (stmt->type == PROCEDURE_DEF)  ? stmt->defproc->id->id : stmt->defun->id->id
#define DEFINITION(stmt) (stmt->type == PROCEDURE_DEF) ? stmt->defproc->def : stmt->defun->def
#define PARAM_DECL(stmt, idx)  (stmt->type == PROCEDURE_DEF) ? stmt->defproc->params[idx] : stmt->defun->params[idx]

#define BLOCK(stmt) stmt->block
#define DECL_ID(stmt) stmt->var_decl->id->id

#define ENTER_SCOPE(scope) scope->depth++
#define EXIT_SCOPE(scope, symtab) scope->depth-- 


// TYPE_CHECK_STMT()
#define GET_SYMBOL_ID(stmt) (stmt->type == FUNCTION_DEF) ?  stmt->defun->id->id  \
                                                            : (stmt->type == PROCEDURE_DEF) ? stmt->defproc->id->id  \
                                                                                        : stmt->defstruct->id->id

#define BUILD_GLOBAL_SYMTAB(symtab, program)  symtab = malloc(sizeof(struct stmt*) * program->n * 2); \
                                                    memset(symtab, 0, sizeof(struct stmt*) * program->n * 2); \
                                                    build_global_symtab(symtab, program)

#define INIT_LOCAL_SYMTAB(symtab, stmt)  symtab->capacity = 32; \
                                                      symtab->n_decls = 0; \
                                                      symtab->decls = malloc(sizeof(struct stmt*) * 32); \
                                                      symtab->func = stmt
#define INIT_SCOPE(scope) scope->depth = 0; \
                                         scope->n_locals = 0; \
                                         scope->capacity = 32; \
                                         scope->locals = malloc(sizeof(struct locals) * 32)
#define FREE(symtab, scope) free(symtab->decls); \
                                free(scope->locals)

// TYPE_CHECK_EXPR()
#define ARITHMETIC(expr) (BIN_OP(expr) == PLUS || BIN_OP(expr) == MINUS \
                                || BIN_OP(expr) == DIVIDE || BIN_OP(expr) == STAR) 
#define COMPARISON(expr) (BIN_OP(expr) == LT || BIN_OP(expr) == GT || BIN_OP(expr) == LTEQ || BIN_OP(expr) == GTEQ)
#define CONNECTIVE(expr) (BIN_OP(expr) == AND || BIN_OP(expr) == OR)
#define EQUALITY(expr)  (BIN_OP(expr) == IS || BIN_OP(expr) == ISNT)
#define FIELD_ACCESS(expr) (BIN_OP(expr) == ARROW)
#define ARRAY_INDEX(expr)  (BIN_OP(expr) == LEFT_BRACK)
#define REQUIRES_INTEGRAL(expr)  (BIN_OP(expr) == BIT_OR || BIN_OP(expr) == BIT_AND || BIN_OP(expr) == SHIFT_R \
                                    || BIN_OP(expr) == SHIFT_L || BIN_OP(expr) == MODULO)


#define BOOLEAN(expr) (expr->indirect == 0 && expr->eval_to == BOOL)
#define ARRAY(expr) (expr->indirect > 0)
#define VALID_STRUCT(expr) (expr->eval_to == STRUCT_ID && expr->indirect == 0)
#define IDENTIFIER(expr)  (expr->type == VARIABLE)
#define VALID_ARITHMETIC_TYPE(expr)  (expr->indirect == 0 && expr->eval_to != STRUCT_ID && expr->eval_to != BOOL)
#define VALID_INTEGRAL_TYPE(expr)   (expr->indirect == 0 && (expr->eval_to == I32 || expr->eval_to = I64))
#define NOT_EQUAL_TYPES(lexpr, rexpr) lexpr->eval_to != rexpr->eval_to



#define BIN_OP(expr) expr->binary->op->type
#define BIN_LEFT(expr) expr->binary->left
#define BIN_RIGHT(expr) expr->binary->right
#define VAR_ID(expr) expr->variable->id->id
#define DECL_TYPE(stmt) stmt->var_decl->type->type
#define DECL_INDIRECT(stmt) stmt->var_decl->indirect
#define DECL_TYPE_ID(stmt) stmt->var_decl->type->id
#define LITERAL_TYPE(expr) expr->literal->value->type
#define N_FIELDS(expr) expr->defstruct->n_fields
#define GET_FIELD(expr, idx) expr->defstruct->fields[idx] 

struct locals {
    int depth;
    char* id;
};

struct scope {
    struct locals* locals;
    int n_locals;
    int capacity;
    int depth;
};

struct local_symtab {
    struct stmt* func;   // for type-checking return statement

    struct stmt** decls;
    int n_decls;
    int capacity;
};

void type_check(struct program* program);
void insert_global_sym(struct stmt** global_symtab, struct stmt* sym, uint64_t idx, int len);
void build_global_symtab(struct stmt** global_symtab, struct program* program);
uint64_t compute_hash(char* id);

#endif
