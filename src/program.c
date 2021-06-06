#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "lexer.h"
#include "parser.h"
#include "type_check.h"

int main(int argc, char* argv[])
{
    FILE* fp = fopen(argv[1], "r");

    if (fp == NULL) {
        fprintf(stderr, "Error opening file \"%s\": %s\n", argv[1], strerror(errno));
        exit(1);
    }

    // later on for #include "file.d" -> just make a set of included files recursively and at the end just 
    // read all the contents of the files into one byte buffer
    // bc compiler doesnt care for declaration order at the global level

    fseek(fp, 0, SEEK_END);
    long f_len = ftell(fp);
    char* src = malloc(f_len);
    rewind(fp);
    fread(src, 1, f_len, fp);
    fclose(fp);

    // refactor lexer/ scanner!
    struct tokens* tokens = scan(src, f_len);
    // free tokens not used 
    struct program* program = parse(tokens);
    //type_check(program);

    // sym_table <- type_check(program)
    // tac <- intermediate_code(sym_table, program)
    // cfg, reverse-cfg <- compute_cfg(tac)
    // live_ranges <- data_flow_analysis(cfg, reverse-cfg)
    // register_allocation(live_ranges, sym_table)
    // code  <- code_gen()

    return 0;
}




    /*
    for (int i = 0; i < tokens->n; i++) {
        switch(tokens->vec[i]->type) {
            case PLUS: printf("PLUS\n"); break;
            case STAR: printf("STAR\n"); break;
            case MODULO: printf("MODULO\n"); break;
            case MINUS: printf("MINUS\n"); break;
            case DIVIDE: printf("DIVIDE\n"); break;
            case LT: printf("LT\n"); break;
            case GT: printf("GT\n"); break;
            case LTEQ: printf("LTEQ\n"); break;
            case GTEQ: printf("GTEQ\n"); break;

            case SHIFT_R: printf("SHIFT_R\n"); break;
            case SHIFT_L: printf("SHIFT_L\n"); break;
            case BIT_NOT: printf("BIT_NOT\n"); break;
            case BIT_OR: printf("BIT_OR\n"); break;
            case BIT_AND: printf("BIT_AND\n"); break;
            case AND: printf("AND\n"); break;
            case OR: printf("OR\n"); break;
            case NOT: printf("NOT\n"); break;
            case IS: printf("IS\n"); break;
            case ISNT: printf("ISNT\n"); break;

            case INTEGER: printf("INTEGER %d\n", tokens->vec[i]->i32); break;
            case LONG: printf("LONG\n"); break;
            case FLOAT: printf("FLOAT\n"); break;
            case DOUBLE: printf("DOUBLE\n"); break;
            case CHARACTER: printf("CHARACTER\n"); break;
            case IDENTIFIER: printf("IDENTIFIER\n"); break;
            case TRUE: printf("TRUE\n"); break;
            case FALSE: printf("FALSE\n"); break;
            case STRING_LITERAL: printf("STRING_LITERAL\n"); break;
            case EMPTY: printf("EMPTY\n"); break;

            case BOOL: printf("BOOL\n"); break;
            case C8: printf("C8\n"); break;
            case I32: printf("I32\n"); break;
            case I64: printf("I64\n"); break;
            case F32: printf("F32\n"); break;
            case F64: printf("F64\n"); break;
            case STRING: printf("STRING\n"); break;
            case STRUCT_ID: printf("STRUCT_ID\n"); break;

            case COLON: printf("COLON\n"); break;
            case ASSIGN: printf("ASSIGN\n"); break;
            case LEFT_PAREN: printf("LEFT_PAREN\n"); break;
            case RIGHT_PAREN: printf("RIGHT_PAREN\n"); break;
            case NEW_LINE: printf("NEW_LINE\n"); break;
            case ARROW: printf("ARROW\n"); break;
            case SPACES: printf("SPACES\n"); break;
            case RIGHT_BRACK: printf("RIGHT_BRACK\n"); break;
            case LEFT_BRACK: printf("LEFT_BRACK\n"); break;

            case ALLOCATE: printf("ALLOCATE\n"); break;
            case STRUCT: printf("STRUCT\n"); break;
            case FUNCTION: printf("FUNCTION\n"); break;
            case PROCEDURE: printf("PROCEDURE\n"); break;
            case COMMA: printf("COMMA\n"); break;
            case IF: printf("IF\n"); break;
            case ELSE: printf("ELSE\n"); break;
            case ELIF: printf("ELIF\n"); break;
            case WHILE: printf("WHILE\n"); break;
            case RETURN: printf("RETURN\n"); break;
        }
    }
    */
