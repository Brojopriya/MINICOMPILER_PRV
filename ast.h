#ifndef AST_H
#define AST_H

#include <stdio.h>

typedef enum {
    AST_PROGRAM,
    AST_DECL,
    AST_STMT_LIST,
    AST_STMT_BLOCK,
    AST_ASSIGN,
    AST_IF,
    AST_WHILE,
    AST_PRINT,
    AST_EXPR_BINOP,
    AST_EXPR_UNOP,
    AST_EXPR_ID,
    AST_EXPR_INT,
    AST_EXPR_BOOL
} NodeType;

typedef enum {
    TYPE_INT,
    TYPE_BOOL
} DataType;

typedef enum {
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
    OP_LT,
    OP_GT,
    OP_EQ,
    OP_NE,
    OP_AND,
    OP_OR
} BinOp;

typedef enum {
    OP_NOT,
    OP_NEG
} UnOp;

struct ASTNode;

typedef struct ASTNode {
    NodeType type;
    DataType data_type;
    int line_no;
    
    union {
        struct {
            struct ASTNode *decl_list;
            struct ASTNode *stmt_list;
        } program;
        
        struct {
            char *id;
            DataType type;
            struct ASTNode *next;
        } decl;
        
        struct {
            struct ASTNode *stmts;
            struct ASTNode *next;
        } stmt_list;
        
        struct {
            struct ASTNode *statements;
        } stmt_block;
        
        struct {
            char *id;
            struct ASTNode *expr;
        } assign;
        
        struct {
            struct ASTNode *cond;
            struct ASTNode *if_block;
            struct ASTNode *else_block;
        } if_stmt;
        
        struct {
            struct ASTNode *cond;
            struct ASTNode *body;
        } while_stmt;
        
        struct {
            struct ASTNode *expr;
        } print_stmt;
        
        struct {
            BinOp op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binop;
        
        struct {
            UnOp op;
            struct ASTNode *operand;
        } unop;
        
        struct {
            char *name;
        } id;
        
        struct {
            int value;
        } int_val;
        
        struct {
            int value;
        } bool_val;
    } data;
} ASTNode;

ASTNode* ast_create_program(ASTNode *decls, ASTNode *stmts);
ASTNode* ast_create_decl(char *id, DataType type, ASTNode *next);
ASTNode* ast_create_stmt_list(ASTNode *stmt, ASTNode *next);
ASTNode* ast_create_stmt_block(ASTNode *stmts);
ASTNode* ast_create_assign(char *id, ASTNode *expr);
ASTNode* ast_create_if(ASTNode *cond, ASTNode *if_block, ASTNode *else_block);
ASTNode* ast_create_while(ASTNode *cond, ASTNode *body);
ASTNode* ast_create_print(ASTNode *expr);
ASTNode* ast_create_binop(BinOp op, ASTNode *left, ASTNode *right);
ASTNode* ast_create_unop(UnOp op, ASTNode *operand);
ASTNode* ast_create_id(char *name);
ASTNode* ast_create_int(int value);
ASTNode* ast_create_bool(int value);
void ast_free(ASTNode *node);
void ast_print(ASTNode *node, FILE *out, int indent);

#endif
