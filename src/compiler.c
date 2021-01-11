
#include <string.h>
#include "compiler.h"
#include "ast.h"





void compile (struct stmt_vector* program, char* f_name)
{
    struct program_info prog_info;
    struct sym_table fnid_map;
    struct scope_info sinfo;

    init_program_info(&prog_info);
    init_sym_table(&fnid_map);
    init_scope_info(&sinfo);


    for (int i = 0; i < program->n_stmts; i++) 
        compile_stmt(program[i], &prog_info);
    // turn file_name from .lov -> .lovc
    // fopen(  , wb);
    // read program info into file.
    
    // main (relative to opcode section)
    // cp_entry (absolute)
    // cp_size
}

void compile_stmt (struct stmt* stmt, struct program_info* prog_info)
{
    switch (stmt->type) {
        case INSTRUCTN_STMT: {
            // store fn id and pos
            // store fn params into scope + 1. 
            // compile body 
        }
        case BLOCK:
        case VAR_DECL:
        case IF_STMT:
        case WHILE_STMT:
        case RETURN_STMT:
        case ASSIGN:
        case OUTPUT_STMT:
    }
}









// program info
void init_program_info (struct program_info* prog_info)
{
    prog_info->bc_pos = 0;
    prog_info->bc_size = 512;
    prog_info->cp_pos = 0;
    prog_info->cp_size = 512;

    prog_info->byte_code = malloc( sizeof(char) * bc_size );
    prog_info->const_pool = malloc( sizeof(char) * cp_size );
}

char* emit_opcode (enum opcode op, uint16_t n_arg_bytes, struct program_info* prog_info)
{
    if (prog_info->bc_pos + n_arg_bytes >= prog_info->bc_size)
        prog_info->byte_code = realloc(prog_info->byte_code, sizeof(char) * (prog_info->bc_size *= 2));

    prog_info->byte_code[prog_info->bc_pos++] = (char)op;
    char* pos_args = prog_info->byte_code + prog_info->bc_pos;
    prog_info->bc_pos += n_arg_bytes;

    return pos_args;
}

void emit_constant (void* const_v, enum token_type const_t, struct program_info* prog_info)
{
    switch (const_t) {
        case INTEGER: {
            if (prog_info->cp_pos + 3 >= prog_info->cp_size)
                prog_info->const_pool = realloc(prog_info->const_pool, sizeof(char) * (prog_info->cp_size *= 2));

            int v = *(int*)const_v;
            char* pos = prog_info->const_pool + prog_info->cp_pos;
            memcpy(pos, &v, 4);
            prog_info->cp_pos += 4;
            break;
        }
        case LONG_INTEGER: {
            if (prog_info->cp_pos + 7 >= prog_info->cp_size)
                prog_info->const_pool = realloc(prog_info->const_pool, sizeof(char) * (prog_info->cp_size *= 2));

            long v = *(long*)const_v;
            char* pos = prog_info->const_pool + prog_info->cp_pos;
            memcpy(pos, &v, 8);
            prog_info->cp_pos += 8;
            break;
        }
        default:
            break;  // unreachable 
    }
}

// sym_table

void init_sym_table (struct sym_table* fnid_map)
{
    fnid_map->n_fnids = 0;
    fnid_map->size = 37
    fnid_map->fnid_info = calloc(37, sizeof(struct fnid_item));
}


int hash_id (char* k, int table_size)
{
    int h = 0;
    int i = 0;

    while (k[i] != '\0') {
        h = (31 * h + k[i]) % table_size;
        i++;
    }
    return h;
}


void insert_fnid (char* id, uint16_t offset, struct sym_table* fnid_map)
{
    if (fnid_map->n_fnids * 1.0 / fnid_map->size > 0.50) {
        int prev_size = fnid_map->size;
        fnid_map->fn_info = realloc(fnid_map->fn_info, sizeof(struct fnid_item) * (fnid_map->size *= 2));
        memset(&(fnid->fn_info + prev_size), 0, sizeof(struct fnid_item) * (fnid_map->size - prev_size));
    }

    int i = hash_id(id, fnid_map->size);
    struct fnid_item* f_item = &fnid_map->fn_info[i];

    while (f_item->is_reserved) {
        if (++i == fnid_map->size) i = 0;
        f_item = &fnid_map->fn_info[i];
    }

    f_item->is_reserved = true;
    f_item->key = id;
    f_item->pos = offset;
}


uint16_t find_fn_pos (char* id, struct sym_table* fnid_map)
{
    int i = hash_id(id, fnid_map->size);
    i = stop;
    struct fnid_item* f_item = &fnid_map->fn_info[i];

    do {
        if (!strcmp(f_item->key, id)) return f_item->pos;
        if (++i == fnid_map->size) i = 0;
        f_item = &fnid_map->fn_info[i];
    } while ( i != stop && f_item->is_reserved );

    return 0; // unreachable -> semantic analyzer should guarantee that functions are defined (temp fix to comply with return type).
}


// scope info

void init_scope_info (struct scope_info* sinfo)
{
    sinfo->n_locals = 0;
    sinfo->depth = 0;
}

void enter_scope (struct scope_info* sinfo) { sinfo->depth++; }

void exit_scope (struct scope_info* sinfo)
{
    struct local* loc_id = &sinfo->locals[sinfo->n_locals-1];

    while (loc_id->depth_declared == sinfo->depth)
        loc_id = &sinfo->locals[--sinfo->n_locals-1];
    sinfo->depth--;
}

void store_local (struct token* local_id, struct scope_info* sinfo) 
{
    struct local* local_var = sinfo->locals[sinfo->n_locals++];
    local_var->id = local_id;
    local_var->depth_declared = sinfo->depth;
}

int load_local (struct token* local_id, struct scope_info* sinfo)
{
    int i;
    for (i = sinfo->n_locals-1; i > -1; i--)
        if (!strcmp(sinfo->locals[i].id->string_v, local_id->string_v))
            return i;

    return -1;  // unreachable -> semantic analyzer should guarantee that variables are defined (temp fix to comply with return type).
}









