#include <stdio.h>
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
        compile_stmt(program->vec[i], &prog_info, &fnid_map, &sinfo);

    int name_length = strlen(f_name);
    char file_c[name_length + 2];
    strncpy(file_c, f_name, name_length);
    file_c[name_length] = 'c';
    file_c[name_length+1] = '\0';

    FILE* class_file = fopen(file_c, "wb");

    int cp_entry = prog_info.bc_pos + 12;
    int cp_length = prog_info.cp_pos;

    memcpy(prog_info.metadata + 4, &cp_entry, 4);
    memcpy(prog_info.metadata + 8, &cp_length, 4);

    fwrite(prog_info.metadata, sizeof(char), 12, class_file);
    fwrite(prog_info.byte_code, sizeof(char), prog_info.bc_pos, class_file);
    fwrite(prog_info.const_pool, sizeof(char), prog_info.cp_pos, class_file);

    fclose(class_file);
}

void compile_stmt (struct stmt* stmt, struct program_info* prog_info,
                   struct sym_table* fnid_map, struct scope_info* sinfo)
{
    switch (stmt->type) {
        case FUNCTION_STMT: {
            struct function_stmt* fn = stmt->function;

            if (fn->id->type == MAIN) {
                int mainf_pos = prog_info->bc_pos;
                memcpy(prog_info->metadata, &mainf_pos, 4);
            }
            
            if (fn->id->type != MAIN) {
                struct fnid_item* fn_item = find_fnid(fn->id->string_v, fnid_map);

                if (!fn_item) {
                    insert_fnid(fn->id->string_v, true, prog_info->bc_pos, fnid_map);

                } else {
                    for (int i = 0; i < fn_item->n_calls; i++)
                        memcpy(fn_item->called_positions[i], &prog_info->bc_pos, 2);

                    fn_item->is_defined = true;
                    fn_item->pos = prog_info->bc_pos;
                }
            }

            if (fn->params != NULL) {
                sinfo->depth++;
                for (int i= fn->params->n_params - 1; i > -1; i--) {
                    store_local(fn->params->vec[i]->decl_stmt->id, sinfo);
                }
                sinfo->depth--;
            }


            compile_stmt(fn->body, prog_info, fnid_map, sinfo);

            if (fn->id->type == MAIN)
                emit_opcode(END, 0, prog_info);
            break;
        }
        case BLOCK: {
            enter_scope(sinfo);

            struct block_stmt* block = stmt->block_stmt;

            for (int i=0; i < block->stmts->n_stmts; i++)
                compile_stmt(block->stmts->vec[i], prog_info, fnid_map, sinfo);

            exit_scope(sinfo);
            break;
        }
        case VAR_DECL: {
            struct var_decl* var = stmt->decl_stmt;

            if (var->value != NULL) {
                compile_expr(var->value, prog_info, fnid_map, sinfo);
                char* arg_pos = emit_opcode(STORE, 1, prog_info);
                memcpy(arg_pos, &sinfo->n_locals, 1); 
            }

            store_local(var->id, sinfo);
            break;
        }
        case IF_STMT: {

            struct if_stmt* ifstmt = stmt->if_stmt;
            compile_expr(ifstmt->cond, prog_info, fnid_map, sinfo);

            char* jmpf_pos = emit_opcode(JMPF, 2, prog_info); 
            compile_stmt(ifstmt->then_branch, prog_info, fnid_map, sinfo);

            if (ifstmt->else_branch != NULL) {
                char* jmp_end = emit_opcode(JMP, 2, prog_info); 
                memcpy(jmpf_pos, &prog_info->bc_pos, 2);
                compile_stmt(ifstmt->else_branch, prog_info, fnid_map, sinfo);
                memcpy(jmp_end, &prog_info->bc_pos, 2);
            } else {
                memcpy(jmpf_pos, &prog_info->bc_pos, 2);
            }

            break;
        }
        case WHILE_STMT: {

            struct while_stmt* wh_stmt = stmt->while_stmt;

            uint16_t jmp_pos = prog_info->bc_pos;
            compile_expr(wh_stmt->cond, prog_info, fnid_map, sinfo);

            char* jmpf_pos = emit_opcode(JMPF, 2, prog_info);
            compile_stmt(wh_stmt->body, prog_info, fnid_map, sinfo);

            char* jmp_arg = emit_opcode(JMP, 2, prog_info);
            memcpy(jmp_arg, &jmp_pos, 2);

            memcpy(jmpf_pos, &prog_info->bc_pos, 2);

            break;
        }
        case RETURN_STMT: {
            struct return_stmt* ret_stmt = stmt->return_stmt;

            compile_expr(ret_stmt->ret_v, prog_info, fnid_map, sinfo);
            emit_opcode(RET, 0, prog_info);
            break;
        }
        case ASSIGN: {
            struct assign_stmt* eq_stmt = stmt->assign_stmt;

            compile_expr(eq_stmt->value, prog_info, fnid_map, sinfo);

            char* arg_pos = emit_opcode(STORE, 1, prog_info);
            int local_pos = load_local(eq_stmt->id, sinfo);
            memcpy(arg_pos, &local_pos, 1);

            break;
        }
        case OUTPUT_STMT: {
            struct output_stmt* ostmt = stmt->output_stmt;
            compile_expr(ostmt->output_v, prog_info, fnid_map, sinfo);
            emit_opcode(OUT, 0, prog_info);
            break;
        }
    }

}


void compile_expr (struct expr* expr, struct program_info* prog_info,
                   struct sym_table* fnid_map, struct scope_info* sinfo)
{
    switch (expr->type) {

        case BINARY: {    // for now just assume INTEGER and support for others later.
                compile_expr(expr->binary->left, prog_info, fnid_map, sinfo);
                compile_expr(expr->binary->right, prog_info, fnid_map, sinfo);

                switch (expr->binary->op->type) {
                    case PLUS: emit_opcode(IADD, 0, prog_info); break;
                    case MINUS: emit_opcode(ISUB, 0, prog_info); break;
                    case ASTERISK: emit_opcode(IMUL, 0, prog_info); break;
                    case SLASH: emit_opcode(IDIV, 0, prog_info); break;
                    case LESS_EQUAL: emit_opcode(LTEQ, 0, prog_info); break;
                    case LESS: emit_opcode(LT, 0, prog_info); break;
                    case GREATER_EQUAL: emit_opcode(GTEQ, 0, prog_info); break;
                    case GREATER: emit_opcode(GT, 0, prog_info); break;
                    case AND: emit_opcode(CAND, 0, prog_info); break;
                    case OR: emit_opcode(COR, 0, prog_info); break;
                    case EQUAL_EQUAL: emit_opcode(EQ, 0, prog_info); break;
                    default: break;
                }

            break;
        }

        case LITERAL: {

                switch (expr->literal->info->type) {
                    case INTEGER: {
                        char* offset = emit_opcode(ICONST, 2, prog_info);
                        memcpy(offset, &prog_info->cp_pos, 2);
                        emit_constant(&expr->literal->info->int_v, INTEGER, prog_info);
                        break;
                    }

                    case LONG_INTEGER: {
                        //char* offset = emit_opcode(LCONST, 2, prog_info);
                        //memcpy(offset, &prog_info->cp_pos, 2);
                        //emit_constant(lit->info->long_v, LONG_INTEGER, prog_info);
                        break;
                    }

                    default:
                        break;
                }
            break;
        }

        case PAREN: 
            compile_expr(expr->grouping->expr, prog_info, fnid_map, sinfo); 
            break;

        case UNARY: {
            compile_expr(expr->unary->right, prog_info, fnid_map, sinfo);

            switch (expr->unary->op->type) {
                case BANG: emit_opcode(NOT, 0, prog_info); break;
                case MINUS: emit_opcode(NEG, 0, prog_info); break;
                default: 
                    break;
            } 
            break;
        }

        case VARIABLE: {
            char* arg_pos = emit_opcode(LOAD, 1, prog_info);
            int local_pos = load_local(expr->variable->id, sinfo);
            memcpy(arg_pos, &local_pos, 1);
            break;
        }

        case CALL: {
            int n_args = expr->call->args->n_args;

            for (int i=0; i < n_args; i++)
                compile_expr( expr->call->args->vec[i], prog_info, fnid_map, sinfo);
            
            char* arg_pos = emit_opcode(CCALL, 3, prog_info);
            char* f_id = expr->call->id->variable->id->string_v;

            struct fnid_item* fn = find_fnid(f_id, fnid_map);

            if (!fn) {
                fn = insert_fnid(f_id, false, 0, fnid_map);
                insert_called_pos(arg_pos, fn);
            } else {
                if (fn->is_defined)
                    memcpy(arg_pos, &fn->pos, 2);
                else
                    insert_called_pos(arg_pos, fn);
            }

            arg_pos += 2;
            memcpy(arg_pos, &n_args, 1);
            break;
        }

    }
}



// program info
void init_program_info (struct program_info* prog_info)
{
    prog_info->bc_pos = 0;
    prog_info->bc_size = 512;
    prog_info->cp_pos = 0;
    prog_info->cp_size = 512;

    prog_info->byte_code = malloc( sizeof(char) * prog_info->bc_size );
    prog_info->const_pool = malloc( sizeof(char) * prog_info->cp_size );
}


char* emit_opcode (enum op_code op, uint16_t n_arg_bytes, struct program_info* prog_info)
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
    fnid_map->size = 37;
    fnid_map->fn_info = calloc(37, sizeof(struct fnid_item));
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


struct fnid_item* insert_fnid (char* id, bool def_status, uint16_t offset, struct sym_table* fnid_map)
{
    if (fnid_map->n_fnids * 1.0 / fnid_map->size > 0.50) {
        int prev_size = fnid_map->size;
        fnid_map->fn_info = realloc(fnid_map->fn_info, sizeof(struct fnid_item) * (fnid_map->size *= 2));
        memset(fnid_map->fn_info + prev_size, 0, sizeof(struct fnid_item) * (fnid_map->size - prev_size));
    }

    int i = hash_id(id, fnid_map->size);
    struct fnid_item* f_item = &fnid_map->fn_info[i];

    while (f_item->is_reserved) {
        if (++i == fnid_map->size) i = 0;
        f_item = &fnid_map->fn_info[i];
    }

    f_item->is_reserved = true;
    f_item->is_defined = def_status;
    f_item->key = id;
    f_item->pos = offset;
    f_item->limit = 20;
    f_item->called_positions = malloc(sizeof(char*) * f_item->limit);
    f_item->n_calls = 0;

    return f_item;
}


struct fnid_item* find_fnid (char* id, struct sym_table* fnid_map)
{
    int i = hash_id(id, fnid_map->size);
    int stop = i;
    struct fnid_item* f_item = &fnid_map->fn_info[i];

    do {
        if (f_item->key != NULL && !strcmp(f_item->key, id)) 
            return f_item;
        if (++i == fnid_map->size) 
            i = 0;
        f_item = &fnid_map->fn_info[i];
    } while ( i != stop && f_item->is_reserved );

    return NULL;
}


void insert_called_pos (char* pos, struct fnid_item* fn) 
{
    if (fn->n_calls == fn->limit)
        fn->called_positions = realloc(fn->called_positions, sizeof(char*) * (fn->limit *= 2));

    fn->called_positions[fn->n_calls++] = pos;
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

    while (loc_id->depth_declared == sinfo->depth && sinfo->n_locals > 0)
        if (sinfo->n_locals != 1) 
            loc_id = &sinfo->locals[--sinfo->n_locals-1];
        else
            sinfo->n_locals--;

    sinfo->depth--;
}

void store_local (struct token* local_id, struct scope_info* sinfo) 
{
    struct local* local_var = &sinfo->locals[sinfo->n_locals++];
    local_var->id = local_id;
    local_var->depth_declared = sinfo->depth;
}

int load_local (struct token* local_id, struct scope_info* sinfo)
{
    int i;
    for (i = sinfo->n_locals-1; i > -1; i--) {
        if (!strcmp(sinfo->locals[i].id->string_v, local_id->string_v))
            return i;
    }

    return -1;  // unreachable -> semantic analyzer should guarantee that variables are defined (temp fix to comply with return type).
}









