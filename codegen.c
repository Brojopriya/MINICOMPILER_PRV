#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

CodeGenerator* codegen_create() {
    CodeGenerator *cg = (CodeGenerator *)malloc(sizeof(CodeGenerator));
    cg->head = NULL;
    cg->tail = NULL;
    cg->temp_counter = 0;
    cg->label_counter = 0;
    
    cg->var_count = 0;
    cg->var_capacity = 16;
    cg->variables = (char **)malloc(cg->var_capacity * sizeof(char *));
    cg->variable_types = (DataType *)malloc(cg->var_capacity * sizeof(DataType));
    
    cg->code_buffer = NULL;
    return cg;
}

void codegen_destroy(CodeGenerator *cg) {
    if (!cg) return;
    
    TACInst *inst = cg->head;
    while (inst) {
        TACInst *next = inst->next;
        if (inst->dest) free(inst->dest);
        if (inst->arg1) free(inst->arg1);
        if (inst->arg2) free(inst->arg2);
        free(inst);
        inst = next;
    }
    
    for (int i = 0; i < cg->var_count; i++) {
        free(cg->variables[i]);
    }
    free(cg->variables);
    free(cg->variable_types);
    
    if (cg->code_buffer) {
        free(cg->code_buffer);
    }
    free(cg);
}

void codegen_add_inst(CodeGenerator *cg, TACOp op, const char *dest, const char *arg1, const char *arg2) {
    TACInst *inst = (TACInst *)malloc(sizeof(TACInst));
    inst->op = op;
    inst->dest = dest ? strdup(dest) : NULL;
    inst->arg1 = arg1 ? strdup(arg1) : NULL;
    inst->arg2 = arg2 ? strdup(arg2) : NULL;
    inst->next = NULL;
    inst->prev = cg->tail;
    
    if (cg->tail) {
        cg->tail->next = inst;
    } else {
        cg->head = inst;
    }
    cg->tail = inst;
}

void codegen_add_var(CodeGenerator *cg, const char *name, DataType type) {
    if (cg->var_count >= cg->var_capacity) {
        cg->var_capacity *= 2;
        cg->variables = (char **)realloc(cg->variables, cg->var_capacity * sizeof(char *));
        cg->variable_types = (DataType *)realloc(cg->variable_types, cg->var_capacity * sizeof(DataType));
    }
    cg->variables[cg->var_count] = strdup(name);
    cg->variable_types[cg->var_count] = type;
    cg->var_count++;
}

char* codegen_new_temp(CodeGenerator *cg) {
    char temp_name[32];
    sprintf(temp_name, "t%d", cg->temp_counter++);
    return strdup(temp_name);
}

char* codegen_new_label(CodeGenerator *cg) {
    char label_name[32];
    sprintf(label_name, "L%d", cg->label_counter++);
    return strdup(label_name);
}

char* codegen_expr(ASTNode *expr, CodeGenerator *cg) {
    if (!expr) return NULL;
    
    switch (expr->type) {
        case AST_EXPR_INT: {
            char val_str[32];
            sprintf(val_str, "%d", expr->data.int_val.value);
            char *temp = codegen_new_temp(cg);
            codegen_add_inst(cg, TAC_ASSIGN, temp, val_str, NULL);
            return temp;
        }
        
        case AST_EXPR_BOOL: {
            char *temp = codegen_new_temp(cg);
            codegen_add_inst(cg, TAC_ASSIGN, temp, expr->data.bool_val.value ? "1" : "0", NULL);
            return temp;
        }
        
        case AST_EXPR_ID: {
            return strdup(expr->data.id.name);
        }
        
        case AST_EXPR_BINOP: {
            char *left = codegen_expr(expr->data.binop.left, cg);
            char *right = codegen_expr(expr->data.binop.right, cg);
            char *temp = codegen_new_temp(cg);
            
            TACOp op = TAC_ADD;
            switch (expr->data.binop.op) {
                case OP_PLUS:  op = TAC_ADD; break;
                case OP_MINUS: op = TAC_SUB; break;
                case OP_MUL:   op = TAC_MUL; break;
                case OP_DIV:   op = TAC_DIV; break;
                case OP_LT:    op = TAC_LT; break;
                case OP_GT:    op = TAC_GT; break;
                case OP_EQ:    op = TAC_EQ; break;
                case OP_NE:    op = TAC_NE; break;
                case OP_AND:   op = TAC_AND; break;
                case OP_OR:    op = TAC_OR; break;
            }
            
            codegen_add_inst(cg, op, temp, left, right);
            free(left);
            free(right);
            return temp;
        }
        
        case AST_EXPR_UNOP: {
            char *operand = codegen_expr(expr->data.unop.operand, cg);
            char *temp = codegen_new_temp(cg);
            
            TACOp op = TAC_NEG;
            if (expr->data.unop.op == OP_NOT) op = TAC_NOT;
            else if (expr->data.unop.op == OP_NEG) op = TAC_NEG;
            
            codegen_add_inst(cg, op, temp, operand, NULL);
            free(operand);
            return temp;
        }
        
        default:
            return NULL;
    }
}

void codegen_stmt(ASTNode *stmt, CodeGenerator *cg) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_STMT_LIST:
            codegen_stmt(stmt->data.stmt_list.stmts, cg);
            codegen_stmt(stmt->data.stmt_list.next, cg);
            break;
            
        case AST_STMT_BLOCK:
            codegen_stmt(stmt->data.stmt_block.statements, cg);
            break;
            
        case AST_ASSIGN: {
            char *expr_val = codegen_expr(stmt->data.assign.expr, cg);
            codegen_add_inst(cg, TAC_ASSIGN, stmt->data.assign.id, expr_val, NULL);
            free(expr_val);
            break;
        }
        
        case AST_IF: {
            char *cond = codegen_expr(stmt->data.if_stmt.cond, cg);
            char *else_label = codegen_new_label(cg);
            char *end_label = codegen_new_label(cg);
            
            codegen_add_inst(cg, TAC_IF_FALSE, else_label, cond, NULL);
            free(cond);
            
            codegen_stmt(stmt->data.if_stmt.if_block, cg);
            codegen_add_inst(cg, TAC_GOTO, end_label, NULL, NULL);
            
            codegen_add_inst(cg, TAC_LABEL, else_label, NULL, NULL);
            if (stmt->data.if_stmt.else_block) {
                codegen_stmt(stmt->data.if_stmt.else_block, cg);
            }
            
            codegen_add_inst(cg, TAC_LABEL, end_label, NULL, NULL);
            free(else_label);
            free(end_label);
            break;
        }
        
        case AST_WHILE: {
            char *loop_label = codegen_new_label(cg);
            char *end_label = codegen_new_label(cg);
            
            codegen_add_inst(cg, TAC_LABEL, loop_label, NULL, NULL);
            char *cond = codegen_expr(stmt->data.while_stmt.cond, cg);
            codegen_add_inst(cg, TAC_IF_FALSE, end_label, cond, NULL);
            free(cond);
            
            codegen_stmt(stmt->data.while_stmt.body, cg);
            codegen_add_inst(cg, TAC_GOTO, loop_label, NULL, NULL);
            
            codegen_add_inst(cg, TAC_LABEL, end_label, NULL, NULL);
            free(loop_label);
            free(end_label);
            break;
        }
        
        case AST_PRINT: {
            char *expr_val = codegen_expr(stmt->data.print_stmt.expr, cg);
            codegen_add_inst(cg, TAC_PRINT, NULL, expr_val, NULL);
            free(expr_val);
            break;
        }
        
        default:
            break;
    }
}

void codegen_generate(ASTNode *ast, CodeGenerator *cg) {
    if (!ast || ast->type != AST_PROGRAM) {
        return;
    }
    
    codegen_add_inst(cg, TAC_NOP, "; Three-Address Code (TAC) Generated", NULL, NULL);
    codegen_add_inst(cg, TAC_NOP, "; Variable declarations:", NULL, NULL);
    
    ASTNode *decl = ast->data.program.decl_list;
    while (decl) {
        if (decl->type == AST_DECL) {
            const char *type_str = (decl->data.decl.type == TYPE_INT) ? "int" : "bool";
            char decl_comment[128];
            sprintf(decl_comment, "; %s %s", type_str, decl->data.decl.id);
            codegen_add_inst(cg, TAC_VAR_DECL, decl->data.decl.id, type_str, NULL);
            
            // Register inside variables table for assembly gen
            codegen_add_var(cg, decl->data.decl.id, decl->data.decl.type);
            
            decl = decl->data.decl.next;
        }
    }
    
    codegen_add_inst(cg, TAC_NOP, "", NULL, NULL);
    codegen_add_inst(cg, TAC_NOP, "; Statements:", NULL, NULL);
    codegen_stmt(ast->data.program.stmt_list, cg);
}

char* codegen_get_code(CodeGenerator *cg) {
    if (cg->code_buffer) {
        free(cg->code_buffer);
        cg->code_buffer = NULL;
    }
    
    // Estimate output size to allocate buffer
    int capacity = 4096;
    char *buffer = (char *)malloc(capacity);
    buffer[0] = '\0';
    int size = 0;
    
    TACInst *inst = cg->head;
    while (inst) {
        char line[256];
        line[0] = '\0';
        
        switch (inst->op) {
            case TAC_NOP:
                if (inst->dest) {
                    sprintf(line, "%s\n", inst->dest);
                } else {
                    sprintf(line, "\n");
                }
                break;
                
            case TAC_VAR_DECL:
                sprintf(line, "; %s %s\n", inst->arg1, inst->dest);
                break;
                
            case TAC_ASSIGN:
                sprintf(line, "%s = %s\n", inst->dest, inst->arg1);
                break;
                
            case TAC_ADD:
                sprintf(line, "%s = %s + %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_SUB:
                sprintf(line, "%s = %s - %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_MUL:
                sprintf(line, "%s = %s * %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_DIV:
                sprintf(line, "%s = %s / %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_NEG:
                sprintf(line, "%s = -%s\n", inst->dest, inst->arg1);
                break;
                
            case TAC_NOT:
                sprintf(line, "%s = !%s\n", inst->dest, inst->arg1);
                break;
                
            case TAC_LT:
                sprintf(line, "%s = %s < %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_GT:
                sprintf(line, "%s = %s > %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_EQ:
                sprintf(line, "%s = %s == %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_NE:
                sprintf(line, "%s = %s != %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_AND:
                sprintf(line, "%s = %s && %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_OR:
                sprintf(line, "%s = %s || %s\n", inst->dest, inst->arg1, inst->arg2);
                break;
                
            case TAC_LABEL:
                sprintf(line, "%s:\n", inst->dest);
                break;
                
            case TAC_IF_FALSE:
                sprintf(line, "if_false %s goto %s\n", inst->arg1, inst->dest);
                break;
                
            case TAC_GOTO:
                sprintf(line, "goto %s\n", inst->dest);
                break;
                
            case TAC_PRINT:
                sprintf(line, "print %s\n", inst->arg1);
                break;
        }
        
        int len = strlen(line);
        if (size + len + 1 > capacity) {
            capacity *= 2;
            buffer = (char *)realloc(buffer, capacity);
        }
        strcat(buffer, line);
        size += len;
        
        inst = inst->next;
    }
    
    cg->code_buffer = buffer;
    return cg->code_buffer;
}
