#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "lexer.h"

void print_tokens (struct token_vector* tkns)
{
    for (int i = 0; i < tkns->n_items; i++) {
        switch (tkns->vec[i].type) {
            case INTEGER: printf("INTEGER %d \n", tkns->vec[i].int_v); break;
            case LONG_INT: printf("LONG_INT %li \n", tkns->vec[i].long_v); break;
            case EOF_F: printf("EOF_F NULL\n"); break;
            case IDENTIFIER: printf("IDENTIFIER %s\n", tkns->vec[i].string_v); break;
            case INT: printf("INT int\n"); break;
            case INSTRUCTN: printf("INSTRUCTN instructn\n"); break;
            case AND: printf("AND and\n"); break;
            case LEFT_BRACE: printf("LEFT_BRACE {\n"); break;
            case RIGHT_BRACE: printf("RIGHT_BRACE }\n"); break;
            case LEFT_PAREN: printf("LEFT_PAREN (\n"); break;
            case RIGHT_PAREN: printf("RIGHT_PAREN )\n"); break;
            case ARROW: printf("ARROW =>\n"); break;
            case IF: printf("IF if\n"); break;
            case RETURN: printf("RETURN return\n"); break;
            case PLUS: printf("PLUS +\n"); break;
            case MINUS: printf("MINUS -\n"); break;
            case LESS_EQUAL: printf("LESS_EQUAL <=\n"); break;
            case SEMICOLON: printf("SEMICOLON ;\n"); break;
            case OUTPUT: printf("OUTPUT output\n"); break;
            case EQUAL: printf("EQUAL =\n"); break;
            default:
                break;
        }
    }
    return;
}

int main(int argc, char* argv[])
{
    FILE* fp = fopen(argv[1], "r");

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    char* str = malloc(fsize);
    fread(str, 1, fsize, fp);
    fclose(fp);

    struct src_string src = {0, 0, fsize, str};
    struct token_vector* tokens = scan(&src);

    print_tokens(tokens);

    return 0;
}


