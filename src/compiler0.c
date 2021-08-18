#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler0.h"


int _push_local(struct exec_stack* stack, struct var_decl* var, int scope) {
    if (stack->n_locals == stack->capacity) {
        stack->locals = realloc(stack->locals, sizeof(struct local) * stack->capacity * 2);
        stack->capacity *= 2;
    }

    int recent_addr = 0;

    if (stack->n_locals != 0) {
        recent_addr = stack->locals[stack->n_locals - 1].stack_addr;
    }

    struct local* local = &stack->locals[stack->n_locals++];
    local->scope = scope;
    local->id = var->id->string;

    int total = 1;
    int align = 1;

    enum token_type type = var->type->type;
    if (type == STRING || type == I64 || type == F64 || var->indirect > 0) {
        total = 8;
        align = 8;
    } else if (type == I32 || type == F32) {
        total = 4;
        align = 4;
    } 

    if (var->value != NULL && var->value->type == ARRAY_LITERAL) {
        total *= var->value->array_literal->n_literals;
    }


    int new_addr = recent_addr + 1;

    while ((new_addr - recent_addr < total) ||
            (new_addr % align != 0)) {
        new_addr++;
    }

    local->stack_addr = new_addr;

    return new_addr * -1;
}


// ok
void write_instruction(struct object_data* data, void* code, int n_bytes) {
    if (data->code_pos + n_bytes > data->code_capacity) {
        data->code = realloc(data->code, data->code_capacity * 2);
        memset(data->code + data->code_capacity, 0, data->code_capacity);
        data->code_capacity *= 2;
    }

    memcpy(data->code + data->code_pos, code, n_bytes);
    data->code_pos += n_bytes;
}

void add_nlist64(struct object_data* data, char* sym, uint8_t type, uint8_t sect, uint64_t value) {
    if (data->sym_pos == data->sym_capacity) {
        data->sym_entries = realloc(data->sym_entries, sizeof(struct nlist_64) * data->sym_capacity * 2);
        data->sym_capacity *= 2;
    }

    struct nlist_64* nlist = &data->sym_entries[data->sym_pos++];
    nlist->n_type = type;
    nlist->n_sect = sect;
    nlist->n_desc = 0;
    nlist->n_value = value;  // address relative to __TEXT, __text

    int len = strlen(sym) + 2;
    if (data->str_pos + len > data->str_capacity) {
        data->str_entries = realloc(data->str_entries, data->str_capacity * 2);
        memset(data->str_entries + data->str_capacity, 0, data->str_capacity);
        data->str_capacity *= 2;
    }

    char* entry = malloc(len);
    entry[0] = '_';
    strcpy(entry + 1, sym);
    memcpy(data->str_entries + data->str_pos, entry, len);
    free(entry);

    nlist->n_strx = data->str_pos;
    data->str_pos += len;
}

// REGISTERS HAS BUG
void compile0_stmt(struct stmt* stmt, struct exec_stack* stack, struct object_data* data, struct stmt** global_symtab) {
    switch (stmt->type) {
        case FUNCTION_DEF: {
            while (data->code_pos % 16 != 0) {
                data->code_pos++;
            }
            uint64_t addr = data->code_pos;

            char _enter[] = {0x55, 0x48, 0x89, 0xe5, 0x48, 0x81, 0xec, 0x00, 0x00, 0x00, 0x00};
            write_instruction(data, _enter, 11);

            char* sub_imm32 = data->code - 4;

            for (int i = 0; i < stmt->defun->n_params; i++) {
                char param_addr = (char)_push_local(stack, stmt->defun->params[i]->var_decl, 1);
                enum token_type param_type = stmt->defun->params[i]->var_decl->type->type;
                int indirect = stmt->defun->params[i]->var_decl->indirect;

                if (param_type == C8 && !indirect) {
                    char rex_b[6] = {0x00, 0x00, 0x00, 0x00, 0x04, 0x04};
                    char reg[6] = {0x38, 0x30, 0x10, 0x08, 0x00, 0x08}; 
                    char inst[4] = {0x40, 0x88, 0x45, 0x00};

                    inst[0] |= rex_b[i];
                    inst[2] |= reg[i];

                    int inst_len = 3;
                    int inst_beg = 1;

                    if (i < 2 || i > 3) {
                        inst_len = 4;
                        inst_beg = 0;
                    }

                    memcpy(inst + 3, &param_addr, 1);
                    write_instruction(data, inst + inst_beg, inst_len);

                } else if (param_type == I32 && !indirect) {
                    char reg[6] = {0x38, 0x30, 0x10, 0x08, 0x00, 0x08};
                    char inst[4] = {0x44, 0x89, 0x45, 0x00};

                    inst[2] |= reg[i];

                    int inst_beg = 1;
                    int inst_len = 3;

                    if (i > 3) {
                        inst_beg = 0;
                        inst_len = 4;
                    }

                    memcpy(inst + 3, &param_addr, 1);
                    write_instruction(data, inst + inst_beg, inst_len);

                } else if (param_type == I64 || indirect > 0
                        || param_type == STRING) {
                    char rex_b[6] = {0x00, 0x00, 0x00, 0x00, 0x04, 0x04}; 
                    char reg[6] = {0x38, 0x30, 0x10, 0x08, 0x00, 0x08};
                    char inst[4] = {0x48, 0x89, 0x45, 0x00};

                    inst[0] |= rex_b[i];
                    inst[2] |= reg[i];

                    memcpy(inst + 3, &param_addr, 1);
                    write_instruction(data, inst, 4);

                } else if (param_type == F32 && !indirect) {
                    char reg[6] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28};
                    char inst[5] = {0xf3, 0x0f, 0x11, 0x45, 0x00};

                    inst[3] |= reg[i];
                    memcpy(inst + 4, &param_addr, 1);
                    write_instruction(data, inst, 5);
                } else if (param_type == F64 && !indirect) {
                    char reg[6] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28};
                    char inst[5] = {0xf2, 0x0f, 0x11, 0x45, 0x00};

                    inst[3] |= reg[i];
                    memcpy(inst + 4, &param_addr, 1);
                    write_instruction(data, inst, 5);
                }
            }

            compile0_stmt(stmt->defun->def, stack, data, global_symtab);

            char _exit[] = {0x48, 0x81, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x5d, 0xc3}; 
            write_instruction(data, _exit, 9);
            char* add_imm32 = data->code - 6;

            if (stack->call_status) {
                while (stack->total_space % 16 != 0) {
                    stack->total_space++;
                }
            }

            memcpy(sub_imm32, &stack->total_space, 4);
            memcpy(add_imm32, &stack->total_space, 4);

            add_nlist64(data, stmt->defun->id->string, 0x0f, 1, addr);
            }
            break;

        case VAR_DECL_STMT:

            // if left_side is array_Literal add to nlist64

            break;

        case PROCEDURE_DEF:
        case PROCEDURAL_CALL:
        case RETURN_STMT:
            // stack->subrout_type 
            // for proc just jump to last instruction (pop rbp, ret)
            // for function mov value into rax then jump to last instruction 
        case ASSIGN_STMT:
        case IF_STMT:
        case WHILE_STMT:
        case BLOCK_STMT:
        default:
            break;
    }
}


void compile0(char* filename, struct program* program, struct stmt** global_symtab) {
    // mach_header needs to modify some fields (sizeofcmds = 80 + 24 + 24 + segment_64.cmdsize);
    struct mach_header_64 mach_header = {0xfeedfacf, 16777223, 3, 1, 4, 0, 0x2000, 0};
    // segment_64 needs to modify some fields (cmdsize, vmsize, fileoff, filesize, nsects)
    struct segment_command_64 segment_64 = {0x19, 72, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0x07, 0x07, 0, 0};

    struct build_version_command build_version = {0x32, 24, 1, 0x000a0f00, 0x000a0f06, 0};

    // symtab needs to modify some fields (symoff, nsyms, stroff, strsize)
    struct symtab_command symtab = {0x2, 24, 0, 0, 0, 0};

    // dysymtab needs to modify some fields (ilocalsym, nlocalsym, iextdefsym, nextdefsym, iundefsym, nundefsym)
    struct dysymtab_command dysymtab = {0xb, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


    struct exec_stack stack = {malloc(sizeof(struct local) * 16), 0, 16, 0, 0, 0};
    struct object_data data = {malloc(1024), malloc(sizeof(struct data_section) * 6), malloc(sizeof(struct relocation_info) * 32),
        malloc(sizeof(struct nlist_64) * 32), malloc(512), 0, 1024, 0, 6, 0, 32, 0, 32, 1, 512};
    memset(data.code, 0, 1024);
    memset(data.str_entries, 0, 512);

    for (int i = 0; i < program->n_stmts; i++) {
        if (program->stmts[i]->type != STRUCT_DEF) {
            if ((program->stmts[i]->type == PROCEDURE_DEF && (program->stmts[i]->defproc->id->type == IDENTIFIER ||
                            program->stmts[i]->defproc->id->type == MAIN)) ||
                    (program->stmts[i]->type == FUNCTION_DEF && program->stmts[i]->defun->id->type == IDENTIFIER)) {
                stack.subrout_type = program->stmts[i]->type;
                compile0_stmt(program->stmts[i], &stack, &data, global_symtab);
                stack.n_locals = 0;
                stack.total_space = 0;
                stack.call_status = 0;
            }
        }
    }

  
    //FILE* o_file = fopen("test.o", "w+");
    //fwrite(data.code, 1, data.code_pos, o_file);
}





// add up all declarations for sub rsp

// relocation_entry addresses are mostly in __TEXT, __text
// insert nlists normally and assign indexes in relocation_entries accordingly
// then at the end when sorting the nlists, update the indexes in relocation_entries to reflect the sorted order
// local symbols needs to be updated at the end, undefined and defined are okay

// change addresses in __DATA, __const (CONST_STR)


/*
struct blob {
    // SECTIONS - HERE?  __TEXT, __literal4 (floats), __TEXT, __literal8 (doubles), __TEXT, __cstring (strings)
    // LATER - __DATA, __const  (array literal - strings)
    // __TEXT, __const (array literal - float, doubles, int, long)
    // relative to __TEXT, __text literal sections have to be aligned to specific addresses
    // function definitions must start on 2^4 byte address, literal8 on 2^3, literal4 on 2^2, cstring on 2^0.

    // BLOB
    char* x86;
    struct relocation_info* relocs;  // reverse lexicographical  (float, double, array_literals, calls)
    // add _ infront of symbols
    struct nlist_64* symtab;  // sorted -> LOCAL (lexicographical), DEFINED (alphabetical), UNDEFINED (alphabetical)
    char* strtab; // index 0 is reserved then its just null-terminated strings
};


*/

