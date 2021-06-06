#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"


#define MAX_ARGS 12
#define MAX_FIELDS 16
// PARSE_EXPR()
#define OP_BINDING_POWER(tokens)  tokens->vec[tokens->i]->lbp
#define TERMINATING_SYMBOL(tokens) (tokens->vec[tokens->i]->type == COMMA || tokens->vec[tokens->i]->type == NEW_LINE \
                                        || tokens->vec[tokens->i]->type == RIGHT_PAREN || tokens->vec[tokens->i]->type == COLON \
                                        || tokens->vec[tokens->i]->type == ASSIGN || tokens->vec[tokens->i]->type == RIGHT_BRACK)

// PARSE()
#define INIT_PROGRAM(prog)  prog->stmts = malloc(sizeof(struct stmts*)  * 512);  \
                            prog->size = 512;  \
                            prog->n = 0

#define OVERFLOW(prog)   (prog->n == prog->size)
#define GROW_MEMORY(prog)     prog->stmts = realloc(prog->stmts, sizeof(struct stmt*) * prog->size * 2); \
                         prog->size *= 2

#define ADD_STATEMENT(prog, stmt)    prog->stmts[prog->n++] = stmt
#define NOT_EOF(tokens) (tokens->i < tokens->n)
#define UNUSED_MEMORY_EXISTS(prog)  prog->n < prog->size
#define SHRINK_MEMORY(prog) prog->stmts = realloc(prog->stmts, sizeof(struct stmt*) * prog->n)

// PARSE_STMT()
#define SKIP_LINE(tokens) tokens->i += 2
#define LOOK_AHEAD(tokens)  tokens->vec[tokens->i + 1]->type
#define SPACE_INDENT(tokens) (tokens->vec[tokens->i]->i32)
#define NEXT_TOKEN_IS_TYPE(tokens)  (NEXT_TOKEN(tokens) == BOOL || NEXT_TOKEN(tokens) == C8 || NEXT_TOKEN(tokens) == I32 || \
                                         NEXT_TOKEN(tokens) == I64 || NEXT_TOKEN(tokens) == F32 || NEXT_TOKEN(tokens) == F64 || \
                                            NEXT_TOKEN(tokens) == STRUCT_ID || NEXT_TOKEN(tokens) == STRING)

#define NEXT_IS_PRIMITIVE_TYPE(tokens) (NEXT_TOKEN(tokens) == C8 || NEXT_TOKEN(tokens) == I32 || NEXT_TOKEN(tokens) == I64 || \
                                            NEXT_TOKEN(tokens) == F32 || NEXT_TOKEN(tokens) == F64)

#define INIT_ASSIGN_STMT(stmt, tokens)  struct assign* assign = malloc(sizeof(struct assign)); \
                                                                assign->lhv = parse_expr(tokens, 0); \
                                                                CONSUME_TOKEN(tokens); \
                                                                assign->rhv = parse_expr(tokens, 0); \
                                                                stmt->assign = assign; \
                                                                stmt->type = ASSIGN_STMT

#define INIT_VAR_DECL(stmt, tokens, scope)  struct var_decl* var_decl = malloc(sizeof(struct var_decl)); \
                                                                        var_decl->indirect = 0; \
                                                                        var_decl->type = CONSUME_TOKEN(tokens); \
                                                                        while (NEXT_TOKEN(tokens) == LEFT_BRACK) { \
                                                                            CONSUME_TOKEN(tokens); \
                                                                            CONSUME_TOKEN(tokens); \
                                                                            var_decl->indirect++; \
                                                                        } \
                                                                        var_decl->id = CONSUME_TOKEN(tokens); \
                                                                        if (scope == 0) { \
                                                                            var_decl->value = NULL; \
                                                                        } else { \
                                                                            CONSUME_TOKEN(tokens); \
                                                                            var_decl->value = parse_expr(tokens, 0); \
                                                                        } \
                                                                        stmt->var_decl = var_decl; \
                                                                        stmt->type = VAR_DECL_STMT

#define INIT_RETURN_STMT(stmt, tokens)  struct ret* ret = malloc(sizeof(struct ret)); \
                                                          CONSUME_TOKEN(tokens); \
                                                          ret->value = parse_expr(tokens, 0); \
                                                          stmt->ret = ret; \
                                                          stmt->type = RETURN_STMT

#define INIT_FUNCTION_DEF(stmt, tokens) struct defun* fn = malloc(sizeof(struct defun)); \
                                                           fn->id = CONSUME_TOKEN(tokens); \
                                                           fn->params = malloc(sizeof(struct stmt*) * MAX_ARGS); \
                                                           fn->n_params = 0; \
                                                           stmt->defun = fn; \
                                                           stmt->type = FUNCTION_DEF 

#define INIT_PROCEDURE_DEF(stmt, tokens) struct defproc* proc = malloc(sizeof(struct defproc)); \
                                                                proc->id = CONSUME_TOKEN(toks); \
                                                                proc->params = malloc(sizeof(struct stmt*) * MAX_ARGS); \
                                                                proc->n_params = 0; \
                                                                stmt->defproc = proc; \
                                                                stmt->type = PROCEDURE_DEF

#define INSERT_PARAM(defun, param)  defun->params[defun->n_params++] = param
#define INSERT_RETURN_TYPE(defun, type)  defun->return_type = type
#define NOT_MAX_PARAMS(defun) (defun->n_params < MAX_ARGS)
#define SHRINK(defun) defun->params = realloc(defun->params, sizeof(struct stmt*) * defun->n_params)
#define INSERT_DEFINITION(defun, block) defun->def = block

#define INIT_BLOCK_STMT(stmt, tokens)  struct block* blk = malloc(sizeof(struct block)); \
                                                           blk->stmts = malloc(sizeof(struct stmt*) * 64); \
                                                           blk->n_stmts = 0; \
                                                           stmt->block = blk; \
                                                           stmt->type = BLOCK_STMT

#define BLOCK_OVERFLOW(blk, capacity) blk->n_stmts == capacity
#define GROW_BLOCK(blk, capacity) blk->stmts = realloc(blk->stmts, sizeof(struct stmt*) * capacity * 2); \
                                                   capacity *= 2
#define INSERT_STMT(blk, stmt) blk->stmts[blk->n_stmts++] = stmt
#define NOT_MAX_CAPACITY(blk, capacity) blk->n_stmts < capacity
#define SHRINK_BLOCK(blk) blk->stmts = realloc(blk->stmts, sizeof(struct stmt*) * blk->n_stmts)

#define INIT_PROCEDURE_CALL(stmt, tokens) struct proc* proc = malloc(sizeof(struct proc)); \
                                                              proc->id = CONSUME_TOKEN(tokens); \
                                                              CONSUME_TOKEN(tokens); \
                                                              proc->args = malloc(sizeof(struct expr*) * MAX_ARGS); \
                                                              proc->n_args = 0; \
                                                              stmt->proc = proc; \
                                                              stmt->type = PROCEDURAL_CALL

#define INIT_WHILE_STMT(stmt, tokens, scope) struct while_stmt* whl = malloc(sizeof(struct while_stmt)); \
                                                               whl->cond = parse_expr(tokens, 0); \
                                                               whl->body = parse_stmt(tokens, scope + 4); \
                                                               stmt->while_stmt = whl; \
                                                               stmt->type = WHILE_STMT

#define INIT_IF_STMT(stmt, tokens, scope) struct if_stmt* _if = malloc(sizeof(struct if_stmt)); \
                                                         _if->cond = parse_expr(tokens, 0); \
                                                         _if->body = parse_stmt(tokens, scope + 4); \
                                                         _if->elifs = malloc(sizeof(struct elif_stmt*) * 2); \
                                                         _if->_else = NULL; \
                                                         _if->n_elifs = 0; \
                                                         stmt->if_stmt = _if; \
                                                         stmt->type = IF_STMT

#define INIT_ELSE_STMT(_if, tokens, scope) struct else_stmt* _else = malloc(sizeof(struct else_stmt)); \
                                                               _else->body = parse_stmt(tokens, scope + 4); \
                                                               _if->_else = _else

#define INIT_ELIF_STMT(_if, tokens, scope) struct elif_stmt* elif = malloc(sizeof(struct elif_stmt)); \
                                                             elif->cond = parse_expr(tokens, 0); \
                                                             elif->body = parse_stmt(tokens, scope + 4); \
                                                             _if->elifs[_if->n_elifs++] = elif

#define INIT_STRUCT_DEF(stmt, tokens) struct defstruct* _struct = malloc(sizeof(struct defstruct)); \
                                                                  _struct->id = CONSUME_TOKEN(tokens); \
                                                                  _struct->fields = malloc(sizeof(struct stmt*) * MAX_FIELDS); \
                                                                  _struct->n_fields = 0; \
                                                                  stmt->defstruct = _struct; \
                                                                  stmt->type = STRUCT_DEF

#define INSERT_FIELD(_struct, field) _struct->fields[_struct->n_fields++] = field
#define NOT_MAX_FIELD(_struct) _struct->n_fields < MAX_FIELDS
#define SHRINK_FIELD(_struct) _struct->fields = realloc(_struct->fields, sizeof(struct stmt*) * _struct->n_fields)

#define BRANCH_OVERFLOW(_if, capacity) _if->n_elifs == capacity
#define GROW_BRANCHES(_if, capacity) _if->elifs = realloc(_if->elifs, sizeof(struct elif_stmt*) * capacity * 2); \
                                                     capacity *= 2
#define NOT_MAX_BRANCHES(_if, capacity) _if->n_elifs < capacity
#define SHRINK_BRANCHES(_if) _if->elifs = realloc(_if->elifs, sizeof(struct elif_stmt*) * _if->n_elifs)


// UTILITIES
#define NEXT_TOKEN(tokens) tokens->vec[tokens->i]->type
#define CONSUME_TOKEN(tokens) tokens->vec[tokens->i++]

// PARSE_NUD(), PARSE_STMT()
#define IS_LITERAL(op)   (op->type == INTEGER || op->type == FLOAT || op->type == DOUBLE || op->type == CHARACTER || \
                         op->type == TRUE || op->type == FALSE || op->type == EMPTY || op->type == STRING_LITERAL)

#define IS_UNARY(op)     (op->type == NOT || op->type == MINUS || op->type == BIT_NOT)
#define IS_IDENTIFIER(op)  (op->type == IDENTIFIER)
#define IS_GROUP(op)      (op->type == LEFT_PAREN)
#define IS_ALLOCATION(op) (op->type == ALLOCATE)


#define INIT_LITERAL_EXPR(expr, val) struct literal* literal = malloc(sizeof(struct literal)); \
                                                               literal->value = val; \
                                                               expr->literal = literal; \
                                                               expr->type = LITERAL 

#define INIT_UNARY_EXPR(expr, _operator, operand)  struct unary* unary = malloc(sizeof(struct unary)); \
                                                         unary->op = _operator; \
                                                         unary->right = operand; \
                                                         expr->unary = unary; \
                                                         expr->type = UNARY

#define INIT_VARIABLE_EXPR(expr, identifier)  struct variable* variable = malloc(sizeof(struct variable)); \
                                                                   variable->id = identifier; \
                                                                   expr->variable = variable; \
                                                                   expr->type = VARIABLE


#define INIT_CALL_EXPR(expr, identifier)  struct call* call = malloc(sizeof(struct call)); \
                                                       call->id = identifier; \
                                                       call->args = malloc(sizeof(struct expr*) * MAX_ARGS); \
                                                       call->n_args = 0; \
                                                       expr->call = call; \
                                                       expr->type = CALL

#define INSERT_ARG(call, arg) call->args[call->n_args++] = arg
#define NOT_MAX_ARGS(call)  (call->n_args < MAX_ARGS)
#define SHRINK_ARGS(call) call->args = realloc(call->args, sizeof(struct expr*) * call->n_args)

// PARSE_LED()
#define INIT_BINARY_EXPR(expr, l, op, r)  struct binary* binary = malloc(sizeof(struct binary)); \
                                                            binary->op = op; \
                                                            binary->left = l; \
                                                            binary->right = r; \
                                                            expr->binary = binary; \
                                                            expr->type = BINARY
struct stmt;
enum expr_type { BINARY, UNARY, LITERAL, CALL, VARIABLE };

struct expr {
    enum expr_type type;
    union {
        struct binary* binary;
        struct unary* unary;
        struct literal* literal;
        struct call* call;
        struct variable* variable;
    };

    enum token_type eval_to;
    char* struct_id;
    int indirect;
};


struct binary {
    struct token* op;
    struct expr* left;
    struct expr* right;
};

struct unary {
    struct token* op;
    struct expr* right;
};

struct literal {
    struct token* value;
};

struct variable {
    struct token* id;
};

struct call {
    struct token* id;
    struct expr** args;
    int n_args;
};

enum stmt_type { STRUCT_DEF, FUNCTION_DEF, PROCEDURE_DEF, PROCEDURAL_CALL, RETURN_STMT, ASSIGN_STMT, IF_STMT, WHILE_STMT, VAR_DECL_STMT, BLOCK_STMT };

struct stmt {
    enum stmt_type type;
    union {
        struct defun* defun;
        struct defproc* defproc;
        struct proc* proc;
        struct ret* ret;
        struct assign* assign;
        struct if_stmt* if_stmt;
        struct while_stmt* while_stmt;
        struct var_decl* var_decl;
        struct block* block;
        struct defstruct* defstruct;
    };
};

struct defstruct {
    struct token* id;
    struct stmt** fields;
    int n_fields;
};

struct else_stmt {
    struct stmt* body;
};

struct elif_stmt {
    struct expr* cond;
    struct stmt* body;
};

struct if_stmt {
    struct expr* cond;
    struct stmt* body;
    struct elif_stmt** elifs;
    struct else_stmt* _else;
    int n_elifs;
};

struct while_stmt {
    struct expr* cond;
    struct stmt* body;
};

struct proc {
    struct token* id;
    struct expr** args;
    int n_args;
};

struct block {
    struct stmt** stmts;
    int n_stmts;
};

struct defproc {
    struct token* id;
    struct stmt** params;
    int n_params;
    struct stmt* def;
};

struct defun {
    struct token* id;
    struct token* return_type;
    struct stmt** params;
    int n_params;
    struct stmt* def;
};

struct assign {
    struct expr* lhv;
    struct expr* rhv;
};

struct var_decl {
    struct token* type;
    struct token* id;
    struct expr* value;
    int indirect;
};

struct ret {
    struct expr* value;
};

struct program {
    struct stmt** stmts;
    int n;
    int size;
};

struct program* parse(struct tokens* toks);
struct stmt* parse_stmt(struct tokens* toks, int scope);
struct expr* parse_led(struct expr* left, struct tokens* toks, struct token* op);
struct expr* parse_nud(struct tokens* toks, struct token* op);
struct expr* parse_expr(struct tokens* toks, int rbp); 

#endif
