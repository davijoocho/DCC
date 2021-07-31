#ifndef TYPE_CHECK_H
#define TYPE_CHECK_H

#define SYMBOL_ID(stmt) (stmt->type == STRUCT_DEF) ? stmt->defstruct->id->string : \
            (stmt->type == FUNCTION_DEF) ? stmt->defun->id->string : \
            stmt->defproc->id->string 


struct locals {
    int depth;
    char* id;
};

struct scope_info {
    struct locals* locals;  // because not required to check for NULL 
    int n_locals;
    int depth;
    int capacity;
};

struct local_symtab {
    struct defun* fn;   // for type-checking return statement
    struct var_decl** decls;
    int n_decls;
    int capacity;
};

uint64_t compute_hash2(char* id);

struct stmt* find_global_sym(struct stmt** global_symtab, char* id, int capacity);
void construct_global_symtab(struct stmt** global_symtab, struct program* program);

void clean_up_scope(struct scope_info* scope, struct local_symtab* symtab);
void push_local(struct scope_info* scope, char* id, int depth);

struct var_decl* find_local_sym(struct local_symtab* symtab, char* id);
void delete_local_sym(struct local_symtab* symtab, char* id);
void grow_local_symtab(struct local_symtab* symtab, int new_capacity);
void insert_local(struct local_symtab* symtab, struct var_decl* var_decl);

void type_check_expr(struct local_symtab* local_symtab, struct stmt** global_symtab,
        struct expr* expr, struct program* program);

void type_check_stmt(struct stmt* stmt, struct local_symtab* local_symtab, 
        struct stmt** global_symtab, struct scope_info* scope, struct program* program);

void semantic_analysis(struct program* program);
#endif
