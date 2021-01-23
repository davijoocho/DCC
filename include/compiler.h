#ifndef COMPILER_H
#define COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include "ast.h"
#include "token.h"

#define MAX_LOCAL 32
enum op_code;

// program info -> ok
struct program_info
{
    char metadata[12];

    uint16_t bc_pos;
    uint16_t bc_size;
    char* byte_code;

    uint16_t cp_pos;
    uint16_t cp_size;
    char* const_pool;
};

void init_program_info (struct program_info* prog_info);
char* emit_opcode (enum op_code op, uint16_t n_arg_bytes, struct program_info* prog_info); 
void emit_constant (void* const_v, enum token_type const_t, struct program_info* prog_info);
// add to constpool and bytecode.


// fnid sym_table
struct fnid_item
{
    bool is_reserved;
    bool is_defined;
    char* key;
    uint16_t pos;
    int n_calls;
    int limit;
    char** called_positions;
};

struct sym_table
{
    int n_fnids;
    int size;
    struct fnid_item* fn_info;
};

int hash_id (char* k, int table_size);
void init_sym_table (struct sym_table* fnid_map);
struct fnid_item* insert_fnid (char* id, bool def_status, uint16_t offset, struct sym_table* fnid_map);
struct fnid_item* find_fnid (char* id, struct sym_table* fnid_map);
void insert_called_pos (char* pos, struct fnid_item* fn);



// scope info

struct local
{
    struct token* id;
    int depth_declared;
};

struct scope_info
{
    struct local locals[MAX_LOCAL];
    int n_locals;
    int depth;
};

void init_scope_info (struct scope_info* sinfo);
void enter_scope (struct scope_info* sinfo);
void exit_scope (struct scope_info* sinfo);
void store_local (struct token* local_id, struct scope_info* sinfo);
int load_local (struct token* local_id, struct scope_info* sinfo);




// compile stuff here
void compile (struct stmt_vector* program, char* f_name);
void compile_stmt (struct stmt* stmt, struct program_info* prog_info,
                   struct sym_table* fnid_map, struct scope_info* sinfo);
void compile_expr (struct expr* expr, struct program_info* prog_info,
                   struct sym_table* fnid_map, struct scope_info* sinfo);



// op code info

enum op_code
{
    IADD = 0x01,
    ISUB = 0x02,
    IMUL = 0x03,
    IDIV = 0x04,
    LOAD = 0x05,
    STORE = 0x06,
    GLOAD = 0x07,
    GSTORE = 0x08,
    ICONST = 0x09, 
    CAND = 0x0A,
    COR = 0x0B,
    NOT = 0x0C,
    NEG = 0x0D,
    JMP = 0x0E,
    JMPF = 0x0F,
    JMPT = 0x10,
    EQ = 0x11,
    GT = 0x12,
    LT = 0x13,
    GTEQ = 0x14,
    LTEQ = 0x15,
    CCALL = 0x16,
    RET = 0x17,
    OUT = 0x18,
    END = 0x19
};





#endif
