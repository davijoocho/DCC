#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "compiler.h"

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
    struct stmt_vector* program = parse(tokens);
    compile(program, argv[1]);

    return 0;
}


