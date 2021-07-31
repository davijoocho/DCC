#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
}


// DRIVER

void type_check_expr(struct local_symtab* local_symtab, struct stmt** global_symtab,
        struct expr* expr, struct program* program) {
    switch (expr->type) {
        case BINARY:
            // IGNORE DOT FOR NOW - NOT SUPPORTING STACK ALLOCATED STRUCTS
            // +, -, /, *, %, [], ->, <<, >>, |, &, or, and, is, isnt, <, >, <=, >=
            break;
        case UNARY:
            // ADDRESS-OF, LOGICAL_NOT, NEGATIVE, DEREFERENCE, TYPE_CAST
            break;
        case CALL:
            // OPEN, MALLOC, REALLOC are not in global_symtab, but we know these exists nonetheless
            // they need special support (parameter types, return type) 
            // set open->eval_to = _FILE ? or STRUCT_ID and then eval_to->struct_id = "_FILE"
            break;
        case VARIABLE:
            break;
        case LITERAL:
            break;
        case ARRAY_LITERAL:
        default:
           break;
    }
}

void type_check_stmt(struct stmt* stmt, struct local_symtab* local_symtab, 
        struct stmt** global_symtab, struct scope_info* scope, struct program* program) {
    switch (stmt->type) {
        case PROCEDURE_DEF:
        case FUNCTION_DEF:
            break;
        case BLOCK_STMT:
            break;
        case RETURN_STMT:
            break;
        case VAR_DECL_STMT:
            // STRINGS ARE TRICKY BC OF INDIRECTION THINGS, THINK ABOUT
            // ARRAY_LITERALS TOO
            // _FILE is not in global_symtab, OPEN returns _FILE
            break;
        case ASSIGN_STMT:
            break;
        case PROCEDURAL_CALL:
            // CLOSE, READ, WRITE, MEMCPY, PRINT, FREE are not in global_symtab, but we know these exists nonetheless
            // they need special support (parameter types) 
            break;
        case IF_STMT:
            break;
        case WHILE_STMT:
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

    struct scope_info* scope = malloc(sizeof(struct scope_info));
    scope->locals = malloc(sizeof(struct locals) * 16);
    scope->n_locals = 0;
    scope->capacity = 16;
    scope->depth = 0;

    for (int i = 0; i < program->n_stmts; i++) {
        if (program->stmts[i]->type != STRUCT_DEF) {  // i.e FUNCTION_DEF, PROCEDURE_DEF
            local_symtab->fn = program->stmts[i]->defun;
            type_check_stmt(program->stmts[i], local_symtab, global_symtab, scope, program);
        }
    }

    free(global_symtab);
    free(local_symtab->decls);
    free(local_symtab);
    free(scope->locals);
    free(scope);
}

