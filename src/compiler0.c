#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler0.h"

// memory leak (registers)

void set_up_registers(struct exec_stack* stack) {
    stack->occupied = malloc(sizeof(struct _register));
    stack->occupied->nxt = NULL;
    stack->not_occupied = malloc(sizeof(struct _register));
    stack->not_occupied->nxt = NULL;
    stack->popped = NULL;

    struct _register* prev = stack->not_occupied;

    for (int i = 0; i < 17; i++) {
        struct _register* _reg = malloc(sizeof(struct _register));
        _reg->reg = i;
        if (i > R11) {
            _reg->xmm = 1;
        } else {
            _reg->xmm = 0;
        }
        struct _register* end = prev->nxt;
        prev->nxt = _reg;
        _reg->nxt = end;
        prev = _reg;
    }
}

void _clean_up_scope(struct exec_stack* stack) {
    while (stack->n_locals > 0) {
        struct local* loc = &stack->locals[stack->n_locals - 1];
        if (loc->scope < stack->scope) {
            return;
        }
        stack->n_locals--;
    }
}

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

enum volatile_registers rand_unoc_register(struct exec_stack* stack, int xmm) {
    struct _register* prev = stack->not_occupied;
    struct _register* unoc = NULL;

    while (prev->nxt->xmm != xmm) {
        prev = prev->nxt;
    }

    unoc = prev->nxt;
    prev->nxt = unoc->nxt;

    unoc->nxt = stack->occupied->nxt;
    stack->occupied->nxt = unoc;

    return unoc->reg;
}

void unoccupy_register(struct exec_stack* stack, enum volatile_registers reg) {
    struct _register* prev = stack->occupied;
    struct _register* occ = NULL;

    while (prev->nxt->reg != reg) {
        prev = prev->nxt;
    }
    occ = prev->nxt;
    prev->nxt = occ->nxt;

    occ->nxt = stack->not_occupied->nxt;
    stack->not_occupied->nxt = occ;
}

void add_reloc_entry(struct object_data* data, int32_t address, int idx, int pc_rel, int length, int external, int type) {
    if (data->reloc_pos == data->reloc_capacity) {
        data->reloc_entries = realloc(data->reloc_entries, sizeof(struct relocation_info) * data->reloc_capacity * 2);
        data->reloc_capacity *= 2;
    }

    struct relocation_info* reloc = &data->reloc_entries[data->reloc_pos++];

    reloc->r_pcrel = pc_rel;
    reloc->r_address = address;
    reloc->r_symbolnum = idx;
    reloc->r_length = length;
    reloc->r_extern = external;
    reloc->r_type = type;
}

void compile0_expr(struct expr* expr, struct exec_stack* stack, struct object_data* data, struct stmt** global_symtab) {
    switch (expr->type) {

        case LITERAL: {
            enum token_type type = expr->literal->value->type;
            switch (type) {

                case STRING_LITERAL: {
                    // HERE.
                }
                    break;
                case FLOAT:
                case DOUBLE: {
                    enum volatile_registers dest = rand_unoc_register(stack, 1);
                    struct data_section* __literal = NULL;

                    int section_idx = 0;
                    enum data_type ltype = LITERAL_4;
                    int width = 4;

                    if (type == DOUBLE) {
                        ltype = LITERAL_8;
                        width = 8;
                    }

                    for (int i = 0; i < data->section_pos; i++) {
                        struct data_section* section = &data->sections[i];
                        section_idx++;
                        if (section->type == ltype) {
                            __literal = section;
                            break;
                        }
                    }

                    if (__literal == NULL) {
                        __literal = &data->sections[data->section_pos++];
                        __literal->data = malloc(512);
                        memset(__literal->data, 0, 512);
                        __literal->type = ltype;
                        __literal->pos = 0;
                        __literal->capacity = 512;
                    }

                    if (__literal->pos + width > __literal->capacity) {
                        __literal->data = realloc(__literal->data, __literal->capacity * 2);
                        memset(__literal->data + __literal->capacity, 0, __literal->capacity);
                        __literal->capacity *= 2;
                    }

                    if (type == FLOAT) {
                        __literal->sectname = "__literal4";
                        __literal->align = 2;
                        __literal->flags = 0x3;
                        memcpy(__literal->data + __literal->pos, &expr->literal->value->f32, 4);
                    } else {
                        __literal->sectname = "__literal8";
                        __literal->align = 3;
                        __literal->flags = 0x4;
                        memcpy(__literal->data + __literal->pos, &expr->literal->value->f64, 8);
                    }


                    char reg[8] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38};
                    char inst[8] = {0xf2, 0x0f, 0x10, 0x05, 0x00, 0x00, 0x00, 0x00};

                    if (type == FLOAT) {
                        inst[0] |= 1;
                    }
                    inst[3] |= reg[dest - XMM0];
                    memcpy(inst + 4, &__literal->pos, 4);
                    __literal->pos += width;

                    write_instruction(data, inst, 8);
                    add_reloc_entry(data, (int32_t) data->code_pos - 4, section_idx + 2, 1, 2, 0, 1);

                    expr->reg_occupied = dest;
                }
                    break;

                case CHARACTER:
                case INTEGER:
                case LONG:
                case EMPTY: {
                    enum volatile_registers dest = rand_unoc_register(stack, 0);
                    char reg[9] = {0x07, 0x06, 0x02, 0x01, 0x00, 0x00, 0x01, 0x02, 0x03};
                    char inst[10] = {0x40, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

                    inst[1] |= reg[dest];
                    int inst_beg = 0;
                    int inst_len = 0;

                    if (type == CHARACTER) {
                        inst[1] ^= 8;
                        inst_beg = 1;
                        inst_len = 2;
                        if (dest > RAX || dest == RDI || dest == RSI) {
                            inst_beg = 0;
                            inst_len = 3;
                            if (dest > RAX) {
                                inst[0] |= 1;
                            } 
                        }
                    } else if (type == INTEGER) {
                        inst_beg = 1;
                        inst_len = 5;
                        if (dest > RAX) {
                            inst_beg = 0;
                            inst_len = 6;
                            inst[0] |= 1;
                        }
                    } else if (type == LONG || type == EMPTY) {
                        inst_beg = 0;
                        inst_len = 10;
                        inst[0] |= 8;
                        if (dest > RAX) {
                            inst[0] |= 1;
                        }
                    }

                    if (type == LONG) {
                        memcpy(inst + 2, &expr->literal->value->i64, 8);
                    } else if (type == INTEGER) {
                        memcpy(inst + 2, &expr->literal->value->i32, 4);
                    } else if (type == CHARACTER) {
                        memcpy(inst + 2, &expr->literal->value->c8, 1);
                    }

                    write_instruction(data, inst + inst_beg, inst_len);
                    expr->reg_occupied = dest;
                }
                    break;
                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
}

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
                    char rex[6] = {0x00, 0x00, 0x00, 0x00, 0x04, 0x04};
                    char reg[6] = {0x38, 0x30, 0x10, 0x08, 0x00, 0x08}; 
                    char inst[4] = {0x40, 0x88, 0x45, 0x00};

                    inst[0] |= rex[i];
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

        case VAR_DECL_STMT: {
            int addr = _push_local(stack, stmt->var_decl, stack->scope);
            if (stmt->var_decl->array_literal) {
                // HERE.
                // could be string array literal if so add to sections (DATA_CONST, cstring)
                // could be floating pointer literal if so add to sections (__TEXT, __const) 
                // add to nlist_64
                // relocation entries.
            } else {
                compile0_expr(stmt->var_decl->value, stack, data, global_symtab);
                enum token_type type = stmt->var_decl->value->eval_to;
                enum volatile_registers reg_occupied = stmt->var_decl->value->reg_occupied;

                if (stmt->var_decl->indirect > 0 || type == STRING || type == EMPTY || type == I64 || type == I32 || type == C8) {
                    char reg[9] = {0x38, 0x30, 0x10, 0x08, 0x00, 0x08, 0x0a, 0x12, 0x00};
                    char inst[7] = {0x40, 0x89, 0x85, 0x00, 0x00, 0x00, 0x00};

                    inst[2] |= reg[reg_occupied];
                    int inst_beg = 0;
                    int inst_len = 0;

                    if (type == C8) {
                        inst_beg = 1;
                        inst_len = 6;
                        inst[1] ^= 1;
                        if (reg_occupied > RAX || reg_occupied == RDI || reg_occupied == RSI) {
                            inst_beg = 0;
                            inst_len = 7;
                            if (reg_occupied > RAX) {
                                inst[0] |= 4;
                            }
                        } 
                    } else if (type == I32) {
                        inst_beg = 1;
                        inst_len = 6;
                        if (reg_occupied > RAX) {
                            inst_beg = 0;
                            inst_len = 7;
                            inst[0] |= 4;
                        }
                    } else if (type == I64 || type == EMPTY) {
                        inst_beg = 0;
                        inst_len = 7;
                        inst[0] |= 8;
                        if (reg_occupied > RAX) {
                            inst[0] |= 4;
                        }
                    }
                        
                    memcpy(inst + 3, &addr, 4);
                    write_instruction(data, inst + inst_beg, inst_len);

                } else if (type == F32 || type == F64) {
                    char reg[8] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38};
                    char inst[8] = {0xf2, 0x0f, 0x11, 0x85, 0x00, 0x00, 0x00, 0x00};

                    if (type == F32) {
                        inst[0] |= 1;
                    }

                    inst[3] |= reg[reg_occupied];
                    memcpy(inst + 4, &addr, 4);
                    write_instruction(data, inst, 8);
                } 

                unoccupy_register(stack, reg_occupied);
            }
        }
            break;

        case PROCEDURE_DEF:
            break;
        case PROCEDURAL_CALL:
            break;
        case RETURN_STMT:
            // stack->subrout_type 
            // for proc just jump to last instruction (pop rbp, ret)
            // for function mov value into rax then jump to last instruction 
            break;
        case ASSIGN_STMT:
            break;
        case IF_STMT:
            break;
        case WHILE_STMT:
            break;
        case BLOCK_STMT:
            stack->scope++;

            for (int i = 0; i < stmt->block->n_stmts; i++) {
                compile0_stmt(stmt->block->stmts[i], stack, data, global_symtab);
            }

            if (stack->locals[stack->n_locals - 1].stack_addr > stack->total_space) {
                stack->total_space = stack->locals[stack->n_locals - 1].stack_addr;
            }

            _clean_up_scope(stack);
            stack->scope--;
            break;
        default:
            break;
    }
}

void compile0(char* filename, struct program* program, struct stmt** global_symtab) {
    struct mach_header_64 mach_header = {0xfeedfacf, 16777223, 3, 1, 4, 0, 0x2000, 0};
    struct segment_command_64 segment_64 = {0x19, 72, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0x07, 0x07, 0, 0};
    struct build_version_command build_version = {0x32, 24, 1, 0x000a0f00, 0x000a0f06, 0};
    struct symtab_command symtab = {0x2, 24, 0, 0, 0, 0};
    struct dysymtab_command dysymtab = {0xb, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    struct exec_stack stack = {malloc(sizeof(struct local) * 16), 0, 16, 0, 0, 0, NULL, NULL, NULL, 0};
    struct object_data data = {malloc(1024), malloc(sizeof(struct data_section) * 5), malloc(sizeof(struct relocation_info) * 32),
        malloc(sizeof(struct relocation_info) * 32), malloc(sizeof(struct nlist_64) * 32), malloc(512), 0, 1024, 0, 0, 32, 0, 32, 0, 32, 1, 512};
    memset(data.code, 0, 1024);
    memset(data.str_entries, 0, 512);
    set_up_registers(&stack);

    for (int i = 0; i < program->n_stmts; i++) {
        if (program->stmts[i]->type != STRUCT_DEF) {
            if ((program->stmts[i]->type == PROCEDURE_DEF && program->stmts[i]->defproc->id->type == IDENTIFIER) ||
                    (program->stmts[i]->type == FUNCTION_DEF && 
                     (program->stmts[i]->defun->id->type == MAIN || program->stmts[i]->defun->id->type == IDENTIFIER))) {
                stack.subrout_type = program->stmts[i]->type;
                compile0_stmt(program->stmts[i], &stack, &data, global_symtab);
                stack.n_locals = 0;
                stack.total_space = 0;
                stack.call_status = 0;
            }
        }
    }

    segment_64.nsects = data.section_pos + 1;
    segment_64.cmdsize = 72 + (80 * segment_64.nsects);
    mach_header.sizeofcmds = 128 + segment_64.cmdsize;   
    segment_64.fileoff = 32 + mach_header.sizeofcmds;
    symtab.strsize = data.str_pos;
    symtab.nsyms = data.sym_pos;

    // dynamic symbol table
    for (int i = 0; i < data.sym_pos; i++) {
        struct nlist_64* sym = &data.sym_entries[i];
        if (sym->n_type == 0x0f) {
            dysymtab.nextdefsym++;
        } else if (sym->n_type == 0x01) {
            dysymtab.nundefsym++;
        } else if (sym->n_type == 0x0e) {
            dysymtab.nlocalsym++;
        }
    }

    dysymtab.iextdefsym = dysymtab.ilocalsym + dysymtab.nlocalsym;
    dysymtab.iundefsym = dysymtab.iextdefsym + dysymtab.nextdefsym;

    struct section_64 sections[segment_64.nsects];
    memset(sections, 0, sizeof(struct section_64) * segment_64.nsects);

    // __TEXT, __text
    // nreloc (n reloc->type != 0)
    // sectname, segname, addr, size, offset, align, reloff (x), nreloc (x), flags, reserved1, reserved2
    struct section_64* text = &sections[0];
    strcpy(text->sectname, "__text");
    strcpy(text->segname, "__TEXT");
    text->offset = segment_64.fileoff;
    text->size = data.code_pos;
    text->align = 4;
    text->flags = 0x80000400;

    // sections
    for (int i = 0; i < data.section_pos; i++) {
        struct data_section* sec = &data.sections[i];
        struct section_64* section = &sections[i + 1];
        struct section_64* prev = &section[i];
        
        if (sec->type == LITERAL_4 || sec->type == LITERAL_8 || sec->type == CSTRING) {
            strcpy(section->sectname, sec->sectname);
            strcpy(section->segname, "__TEXT");
            int mod = 1;

            if (sec->type == LITERAL_8) {
                mod = 8;
            } else if (sec->type == LITERAL_4) {
                mod = 4;
            }

            while (data.code_pos % mod != 0) {
                data.code_pos++;
            }

            section->addr = data.code_pos;
            section->size = sec->pos;
            section->flags = sec->flags;
            section->align = sec->align;
            section->offset = text->offset + section->addr;

            if (data.code_pos + sec->pos > data.code_capacity) {
                data.code = realloc(data.code, data.code_capacity * 2);
                memset(data.code + data.code_capacity, 0, data.code_capacity);
                data.code_capacity *= 2;
            }

            memcpy(data.code + data.code_pos, sec->data, sec->pos);
            data.code_pos += sec->pos;

        } else if (sec->type == LCONST) {
            // HERE.
        } else if (sec->type == DATA_CONST) {
            // HERE.
        }
    }

    segment_64.vmsize = segment_64.filesize = data.code_pos;
    text->reloff = text->offset + data.code_pos;
    
    symtab.symoff = text->reloff + (data.reloc_pos + data.data_reloc_pos) * sizeof(struct relocation_info);
    symtab.stroff = symtab.symoff + data.sym_pos * sizeof(struct nlist_64);

    for (int i = 0; i < data.reloc_pos; i++) {
        struct relocation_info* reloc = &data.reloc_entries[i];
        text->nreloc++;
        if (reloc->r_type == 1 && reloc->r_extern == 0 && reloc->r_length == 2 && reloc->r_pcrel == 1) {
            int relative;
            int base = reloc->r_address + 4;
            int target = sections[reloc->r_symbolnum - 1].addr;
            memcpy(&relative, data.code + reloc->r_address, 4);

            int updated_addr = target - base + relative;
            memcpy(data.code + reloc->r_address, &updated_addr, 4);
        }
    }


    /* __DATA, __const
    for (int i = 0; i < data.data_reloc_pos; i++) {
        struct relocation_info* reloc = &data.data_reloc_entries[i];
        struct section_64* data_const = &sections[reloc->r_symbolnum - 1];
    }
    memcpy
    */

    int total_entries = (data.reloc_pos * sizeof(struct relocation_info)) + (data.data_reloc_pos * sizeof(struct relocation_info)) 
        + (data.sym_pos * sizeof(struct nlist_64)) + data.str_pos;

    if (data.code_pos + total_entries > data.code_capacity) {
        data.code = realloc(data.code, data.code_capacity * 2);
        memset(data.code + data.code_capacity, 0, data.code_capacity);
        data.code_capacity *= 2;
    }

    // sort nlist and update indexes relocation_entries

    memcpy(data.code + data.code_pos, data.reloc_entries, data.reloc_pos * sizeof(struct relocation_info));
    data.code_pos += data.reloc_pos * sizeof(struct relocation_info);

    memcpy(data.code + data.code_pos, data.data_reloc_entries, data.data_reloc_pos * sizeof(struct relocation_info));
    data.code_pos += data.data_reloc_pos * sizeof(struct relocation_info);

    memcpy(data.code + data.code_pos, data.sym_entries, data.sym_pos * sizeof(struct nlist_64));
    data.code_pos += data.sym_pos * sizeof(struct nlist_64);

    memcpy(data.code + data.code_pos, data.str_entries, data.str_pos);
    data.code_pos += data.str_pos;

    char* alias = malloc(strlen(filename) + 1);
    strcpy(alias, filename);
    alias[strlen(filename) - 1] = 'o';

    FILE* output = fopen(alias, "w+");
    fwrite(&mach_header, 1, 32, output);
    fwrite(&segment_64, 1, 72, output);
    fwrite(sections, 1, sizeof(sections), output);
    fwrite(&build_version, 1, 24, output);
    fwrite(&symtab, 1, 24, output);
    fwrite(&dysymtab, 1, 80, output);
    fwrite(data.code, 1, data.code_pos, output);
    fclose(output);
}


// insert nlists normally and assign indexes in relocation_entries (function calls, array literals)
// then at the end when sorting the nlists, update the indexes in relocation_entries to reflect the sorted order
// local symbols needs to be updated at the end, undefined and defined are okay (nlist)

// change addresses in __DATA, __const (CONST_STR)


// 1. sort nlists (local - lexicographical, defined - alphabetical, undefined - alphabetical)
// 2. update indexes in relocation_entries (function calls, array literals)
// 3. assign addresses to sections
// 4. while assigning addresses to sections, update addresses in __text (floats, doubles, strings)


/*
struct blob {
    // SECTIONS - HERE?  __TEXT, __literal4 (floats), __TEXT, __literal8 (doubles), __TEXT, __cstring (strings)
    // LATER - __DATA, __const  (array literal - strings)
    // __TEXT, __const (array literal - float, doubles, int, long)
    // relative to __TEXT, __text literal sections have to be aligned to specific addresses
    // function definitions must start on 2^4 byte address, literal8 on 2^3, literal4 on 2^2, cstring on 2^0.
};


*/

