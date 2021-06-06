#ifndef LEXER_H
#define LEXER_H

#define N_KEYWORDS 24
#define IS_ALPHA(c)   (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_')
#define IS_NUMERIC(c) ('0' <= c && c <= '9')
#define TOKEN_OVERFLOW(tokens) tokens->n == tokens->size
#define GROW_TOKEN_SPACE(tokens) tokens->vec = realloc(tokens->vec, sizeof(struct token*) * toks->size * 2); \
                                               tokens->size *= 2
#define PREV_TOKEN(tokens)  tokens->vec[tokens->n - 1]

#define INIT_TOKEN(token, tok_type, binding_power, line_num)   token->type = tok_type; \
                                                                   token->lbp = binding_power; \
                                                                   token->line = line_num

#define ASSIGN_LITERAL_VALUE(token, type, s_md) switch (type) { \
                                        case SPACES: \
                                            token->i32 = s_md->end - s_md->start; \
                                            break; \
                                        case CHARACTER: \
                                            token->c8 = s_md->src[s_md->end]; \
                                            s_md->end += 2; \
                                            break; \
                                        case INTEGER: token->i32 = atoi(s_md->src + s_md->start); break; \
                                        case LONG: token->i64 = strtol(s_md->src + s_md->start, NULL, 10); break; \
                                        case FLOAT: token->f32 = strtof(s_md->src + s_md->start, NULL); break; \
                                        case DOUBLE: token->f64 = strtod(s_md->src + s_md->start, NULL); break; \
                                        case IDENTIFIER: \
                                            token->id = malloc(len + 1); \
                                            strncpy(token->id, s_md->src + s_md->start, len); \
                                            token->id[len] = '\0'; \
                                            break; \
                                        case STRING_LITERAL: \
                                            token->id = malloc(len + 1); \
                                            strncpy(token->id, s_md->src + s_md->start + 1, len); \
                                            token->id[len] = '\0'; \
                                            break; \
                                        default: \
                                            break; \
                                    } 

enum token_type {
    PLUS, STAR, MODULO, MINUS, DIVIDE, LT, GT, LTEQ, GTEQ,
    SHIFT_R, SHIFT_L, BIT_NOT, BIT_OR, BIT_AND, AND, OR, NOT, IS, ISNT, 

    CHARACTER, INTEGER, LONG, FLOAT, DOUBLE, IDENTIFIER,
    TRUE, FALSE, STRING_LITERAL, EMPTY,

    BOOL, C8, I32, I64, F32, F64, STRING, STRUCT_ID,

    COLON, ASSIGN, LEFT_PAREN, RIGHT_PAREN, NEW_LINE, ARROW, // NEW_LINE = 41
    SPACES, RIGHT_BRACK, LEFT_BRACK, 

    ALLOCATE, 
     
    STRUCT, FUNCTION, PROCEDURE, COMMA,

    IF, ELSE, ELIF, WHILE, RETURN
};

struct token {
    enum token_type type;
    union {
        char* id;
        char c8;
        int i32;
        long i64;
        float f32;
        double f64;
    };
    int lbp;
    int line;
};

struct scan_md {
    char* src;
    int line;
    int start;
    int end;
};

struct tokens {
    struct token** vec;
    int n;
    int i;
    int size;
};

int compute_perfect_hash(char* s, int len);
int get_token_type(char* s, int len, char** kw_map);
void add_token(struct tokens* toks, enum token_type type, struct scan_md* s_md, int lbp, long len);
void read_token(struct scan_md* s_md, struct tokens* toks, long len, char** kw_map);
struct tokens* scan(char* src, long len);


#endif
