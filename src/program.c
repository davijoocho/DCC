#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "type_check.h"
#include "compiler0.h"


int main(int argc, char* argv[])
{
    FILE* fp = fopen(argv[1], "r");

    char* stdlib = "fn malloc(i64 n_bytes) -> void*:\nproc free(void* ptr):\n";
    fseek(fp, 0, SEEK_END);
    long src_len = ftell(fp);
    int lib_len = strlen(stdlib);
    char* src = malloc(src_len + lib_len);
    rewind(fp);
    fread(src, 1, src_len, fp);
    fclose(fp);
    strncpy(src + src_len, stdlib, lib_len);

    struct tokens* tokens = lexical_analysis(src, src_len + lib_len);

    /*
    char* types[] = { "LOGICAL_OR", "LOGICAL_AND", "BIT_OR", "AND", 
        "IS", "ISNT","LT", "LTEQ", "GT", "GTEQ", "BIT_SHIFTR", "BIT_SHIFTL",
        "PLUS", "MINUS","STAR", "DIVIDE", "MODULO", "LOGICAL_NOT","LEFT_PAREN",
        "DOT", "ARROW", "LEFT_BRACK", "RIGHT_BRACK", "RIGHT_PAREN", "NEW_LINE", "COMMA", "COLON",
        "CHARACTER", "INTEGER", "LONG", "FLOAT", "DOUBLE", "IDENTIFIER","STRING_LITERAL", "EMPTY",
        "RIGHT_BRACE", "LEFT_BRACE","C8", "I32", "I64", "F32", "F64", "STRING",
        "FUNCTION", "STRUCT", "STRUCT_ID", "ASSIGN", "INDENT", "PROCEDURE", "MAIN",
        "IF", "ELIF", "ELSE", "WHILE", "RETURN", "FREE", "OPEN", "WRITE", "READ", "CLOSE", "MALLOC",
        "MEMCPY", "PRINT" ,"REALLOC", "EOFF", "VOID"};
    for (int i = 0; i < tokens->n_tokens; i++) {
        printf("%s, %s\n", types[tokens->tokens[i]->type], tokens->tokens[i]->lexeme);
    }*/

    // free unused tokens in syntax_analysis
    // free all new_lines
    struct program* program = syntax_analysis(tokens);
    struct stmt** global_symtab = semantic_analysis(program);
    compile0(argv[1], program, global_symtab);

    // free tokens not used 
    //struct program* program = parse(tokens);

    // save type check for later... 
    // implement code generation and register allocator first?
    //type_check(program);

    // sym_table <- type_check(program)
    // tac <- intermediate_code(sym_table, program)
    // cfg, reverse-cfg <- compute_cfg(tac)
    // live_ranges <- data_flow_analysis(cfg, reverse-cfg)
    // register_allocation(live_ranges, sym_table)
    // code  <- code_gen()

    return 0;
}


