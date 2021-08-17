#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#define N_KEYWORDS 31
#define KEYWORD_HASHTAB_SIZE (N_KEYWORDS * 2)
#define IS_NUMERIC(c) ('0' <= c && c <= '9')
#define IS_ALPHA(c) (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_')
#define STRING_LEN(scan_info) (scan_info->end - scan_info->start - 2)

enum token_type {

    // BINARY
    LOGICAL_OR,                          
    LOGICAL_AND,                        
    BIT_OR, 
    AND, 
    IS, ISNT, 
    LT, LTEQ, GT, GTEQ, 
    BIT_SHIFTR, BIT_SHIFTL,  
    PLUS, MINUS, 
    STAR, DIVIDE, MODULO,  // 16

    // UNARY/ NUD
    LOGICAL_NOT,
    LEFT_PAREN,   
    // Negative    (MINUS)
    // DEREFERENCE (STAR)
    // Address-Of  (AND)

    //CALL?

    // HIGHEST PRECEDENCE
    DOT, ARROW, LEFT_BRACK, 

    RIGHT_BRACK, RIGHT_PAREN, NEW_LINE, COMMA, COLON,  // 26

    // LITERALS
    CHARACTER, INTEGER, LONG, FLOAT, DOUBLE, IDENTIFIER,
    STRING_LITERAL, EMPTY,

    RIGHT_BRACE, LEFT_BRACE,

    // TYPES
    C8, I32, I64, F32, F64, STRING,   // 37 - 42

    FUNCTION, STRUCT, STRUCT_ID, ASSIGN, INDENT, 
    PROCEDURE, MAIN,  // 43 - 49 

    IF, ELIF, ELSE, WHILE, RETURN, // 50 - 54

    // STD LIBRARY 
    FREE, OPEN, WRITE, READ, CLOSE, MALLOC, 
    MEMCPY, PRINT, REALLOC,  // 55 - 63

    EOFF, VOID  // 64
};

struct token {
    enum token_type type;
    union {
        char* string;
        char c8;
        int i32;
        long i64;
        float f32;
        double f64;
    };
    char* lexeme;
    int lbp;
    int line;
};

struct tokens {
    struct token** tokens;
    int n_tokens;
    int capacity;
    int idx;
};

struct scanner_info {
    char* source;
    int start;
    int end;
    int line;
};

struct keyword_entry {
    int occupied;
    char* keyword;
    enum token_type type;
};

uint32_t compute_hash(char* str, int len);
void construct_keyword_hashtab(struct keyword_entry* keyword_hashtab);
void add_token(enum token_type type, int lbp, struct scanner_info* scan_info, struct tokens* token_lst);
void scan(struct scanner_info* scan_info, struct keyword_entry* keyword_hashtab, struct tokens* token_lst);
struct tokens* lexical_analysis(char* source, long len);


#endif
