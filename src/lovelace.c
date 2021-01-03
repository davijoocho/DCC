
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

void read_file (char* fname, long* fsize, char* str)
{
    FILE* fp = fopen(fname, "r");
    fseek(fp, 0, SEEK_END);
    *fsize = ftell(fp);
    rewind(fp);

    str = malloc(fsize);
    fread(str, 1, fsize, fp);
    fclose(fp);
};

int main(int argc, char* argv[])
{
    char* str;
    long fsize;

    read_file(argv[1], &fsize, str);
    struct src_string src = {0, 0, fsize, str};

    struct token_vector* tokens = scan(src);

}


