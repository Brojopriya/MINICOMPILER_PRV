#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

typedef enum {
    TAC_NOP,
    TAC_VAR_DECL,  // ; int x / ; bool y
    TAC_ASSIGN,    // dest = arg1
    TAC_ADD,       // dest = arg1 + arg2
    TAC_SUB,       // dest = arg1 - arg2
    TAC_MUL,       // dest = arg1 * arg2
    TAC_DIV,       // dest = arg1 / arg2
    TAC_NEG,       // dest = -arg1
    TAC_NOT,       // dest = !arg1
    TAC_LT,        // dest = arg1 < arg2
    TAC_GT,        // dest = arg1 > arg2
    TAC_EQ,        // dest = arg1 == arg2
    TAC_NE,        // dest = arg1 != arg2
    TAC_AND,       // dest = arg1 && arg2
    TAC_OR,        // dest = arg1 || arg2
    TAC_LABEL,     // dest:
    TAC_IF_FALSE,  // if_false arg1 goto dest
    TAC_GOTO,      // goto dest
    TAC_PRINT      // print arg1
} TACOp;

typedef struct TACInst {
    TACOp op;
    char *dest;
    char *arg1;
    char *arg2;
    struct TACInst *next;
    struct TACInst *prev;
} TACInst;

typedef struct {
    TACInst *head;
    TACInst *tail;
    int temp_counter;
    int label_counter;
    
    // Global variable tracking (essential for target assembly mapping)
    char **variables;
    DataType *variable_types;
    int var_count;
    int var_capacity;
    
    char *code_buffer;
} CodeGenerator;

CodeGenerator* codegen_create();
void codegen_destroy(CodeGenerator *cg);
void codegen_add_inst(CodeGenerator *cg, TACOp op, const char *dest, const char *arg1, const char *arg2);
char* codegen_new_temp(CodeGenerator *cg);
char* codegen_new_label(CodeGenerator *cg);
void codegen_generate(ASTNode *ast, CodeGenerator *cg);
char* codegen_get_code(CodeGenerator *cg);
void codegen_add_var(CodeGenerator *cg, const char *name, DataType type);

#endif
