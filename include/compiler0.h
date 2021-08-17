#ifndef COMPILER0_H
#define COMPILER0_H

#include <stdint.h>
#include "parser.h" 
#include "type_check.h"

struct relocation_info {
    int32_t r_address; 
    uint32_t r_symbolnum:24, 
    r_pcrel:1,
    r_length:2,
    r_extern:1,
    r_type:4;
};

struct nlist_64
{
    uint32_t n_strx;
    uint8_t n_type;
    uint8_t n_sect;
    uint16_t n_desc;
    uint64_t n_value;
};

struct mach_header_64
{
    uint32_t magic;
    int cputype;
    int cpusubtype; 
    uint32_t filetype;     
    uint32_t ncmds;        
    uint32_t sizeofcmds;   
    uint32_t flags;        
    uint32_t reserved;     
};

struct segment_command_64 {  
    uint32_t cmd;   
    uint32_t cmdsize;  // 72 + (80 * n_sects)
    char segname[16]; 
    uint64_t vmaddr;  
    uint64_t vmsize;  // same as filesize 
    uint64_t fileoff;  // offset to machine code
    uint64_t filesize; // start of machine code to end of data/ literal section
    int maxprot; 
    int initprot; 
    uint32_t nsects;   
    uint32_t flags; 
};

struct section_64 {
    char sectname[16];
    char segname[16]; 
    uint64_t addr; 
    uint64_t size; 
    uint32_t offset; 
    uint32_t align; 
    uint32_t reloff; 
    uint32_t nreloc; 
    uint32_t flags; 
    uint32_t reserved1; 
    uint32_t reserved2; 
    // c8 padding[4];
};

struct build_version_command {
    uint32_t	cmd;		
    uint32_t	cmdsize;	
    uint32_t	platform;	
    uint32_t	minos;		
    uint32_t	sdk;		
    uint32_t	ntools;	
};

struct symtab_command {
    uint32_t cmd;            
    uint32_t cmdsize;       
    uint32_t symoff; 
    uint32_t nsyms; 
    uint32_t stroff; 
    uint32_t strsize;
};

struct dysymtab_command {
    uint32_t cmd;           
    uint32_t cmdsize;       

    // local (typically array/ struct literals)
    uint32_t ilocalsym; 
    uint32_t nlocalsym; 

    // defined
    uint32_t iextdefsym;
    uint32_t nextdefsym; 

    // undefined
    uint32_t iundefsym;     
    uint32_t nundefsym; 

    uint32_t tocoff; 
    uint32_t ntoc;
    uint32_t modtaboff; 
    uint32_t nmodtab; 
    uint32_t extrefsymoff; 
    uint32_t nextrefsyms; 
    uint32_t indirectsymoff; 
    uint32_t nindirectsyms; 
    uint32_t extreloff; 
    uint32_t nextrel; 
    uint32_t locreloff; 
    uint32_t nlocrel;
};


enum data_type { LITERAL_4, LITERAL_8, CSTRING, CONST_4, CONST_8, CONST_STRING };
enum registers { RDI, RSI, RDX, RCX, R8, R9, R10 };

struct local {
    char* id;
    int scope;
    int stack_addr;
};

struct exec_stack {
    struct local* locals;
    int n_locals;
    int capacity;
    int scope;

    int total_space;
    int call_status;

    enum registers occupied_regs[7];
    enum registers free_regs[7];
    enum stmt_type subrout_type;
};

struct data_section {
    char* data;
    enum data_type type;
    int capacity;
};

struct object_data {
    char* code;
    struct data_section* section;   
    struct relocation_info* reloc_entries;
    struct nlist_64* sym_entries;
    char* str_entries;

    uint64_t code_pos;
    uint64_t code_capacity;

    int data_pos;
    int data_capacity;

    int reloc_pos;
    int reloc_capacity;

    int sym_pos;
    int sym_capacity;

    uint32_t str_pos;
    uint32_t str_capacity;
};

int _push_local(struct exec_stack* stack, struct var_decl* var, int scope);
void add_nlist64(struct object_data* data, char* sym, uint8_t type, uint8_t sect, uint64_t value);
void write_instruction(struct object_data* data, void* code, int n_bytes);
void compile0_stmt(struct stmt* stmt, struct exec_stack* stack, struct object_data* data, struct stmt** global_symtab);
void compile0(char* filename, struct program* program, struct stmt** global_symtab);


#endif
