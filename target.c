#include "target.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_constant(const char *str) {
    if (!str) return 0;
    int i = 0;
    if (str[0] == '-' && str[1] != '\0') i = 1;
    for (; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') return 0;
    }
    return 1;
}

static int is_label(const char *str) {
    return str && str[0] == 'L' && str[1] >= '0' && str[1] <= '9';
}

static void add_unique_var(char **unique_vars, int *count, const char *name) {
    if (!name || is_constant(name) || is_label(name) || strcmp(name, "") == 0) return;
    
    // Check if already in unique_vars
    for (int i = 0; i < *count; i++) {
        if (strcmp(unique_vars[i], name) == 0) return;
    }
    
    // Add to unique list
    unique_vars[*count] = strdup(name);
    (*count)++;
}

static void load_op(char *buffer, const char *reg, const char *operand) {
    if (is_constant(operand)) {
        sprintf(buffer, "    LI %s, %s\n", reg, operand);
    } else {
        sprintf(buffer, "    LOAD %s, %s\n", reg, operand);
    }
}

char* target_generate_assembly(CodeGenerator *cg) {
    if (!cg) return NULL;
    
    // 1. Collect all unique variables and temporaries from the optimized instructions
    char *unique_vars[512];
    int unique_count = 0;
    
    TACInst *inst = cg->head;
    while (inst) {
        if (inst->op != TAC_NOP && inst->op != TAC_VAR_DECL && inst->op != TAC_LABEL && inst->op != TAC_GOTO) {
            if (inst->dest) add_unique_var(unique_vars, &unique_count, inst->dest);
            if (inst->arg1) add_unique_var(unique_vars, &unique_count, inst->arg1);
            if (inst->arg2) add_unique_var(unique_vars, &unique_count, inst->arg2);
        }
        inst = inst->next;
    }
    
    // 2. Allocate output assembly buffer
    int capacity = 8192;
    char *buffer = (char *)malloc(capacity);
    buffer[0] = '\0';
    int size = 0;
    
    char line[512];
    
    // 3. Emit Data Segment
    sprintf(line, "; ========================================\n");
    strcat(buffer, line);
    sprintf(line, "; Target Pseudo-Assembly Code Generated\n");
    strcat(buffer, line);
    sprintf(line, "; ========================================\n\n");
    strcat(buffer, line);
    
    sprintf(line, ".data\n");
    strcat(buffer, line);
    size = strlen(buffer);
    
    for (int i = 0; i < unique_count; i++) {
        sprintf(line, "%s: .word 0\n", unique_vars[i]);
        if (size + (int)strlen(line) + 1 > capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }
        strcat(buffer, line);
        size += (int)strlen(line);
        free(unique_vars[i]); // clean up
    }
    
    sprintf(line, "\n.text\nmain:\n");
    strcat(buffer, line);
    size = strlen(buffer);
    
    // 4. Emit Text Segment by translating TAC Insts
    inst = cg->head;
    while (inst) {
        line[0] = '\0';
        char op_part[256];
        op_part[0] = '\0';
        
        switch (inst->op) {
            case TAC_NOP:
                if (inst->dest && inst->dest[0] == ';') {
                    sprintf(line, "    %s\n", inst->dest);
                }
                break;
                
            case TAC_VAR_DECL:
                // Comments in assembly
                sprintf(line, "    ; Declare variable: %s %s\n", inst->arg1, inst->dest);
                break;
                
            case TAC_ASSIGN:
                load_op(op_part, "R0", inst->arg1);
                sprintf(line, "%s    STORE %s, R0\n", op_part, inst->dest);
                break;
                
            case TAC_ADD:
            case TAC_SUB:
            case TAC_MUL:
            case TAC_DIV:
            case TAC_LT:
            case TAC_GT:
            case TAC_EQ:
            case TAC_NE:
            case TAC_AND:
            case TAC_OR: {
                load_op(op_part, "R0", inst->arg1);
                char op_part2[128];
                load_op(op_part2, "R1", inst->arg2);
                
                const char *asm_op = "ADD";
                switch (inst->op) {
                    case TAC_ADD: asm_op = "ADD"; break;
                    case TAC_SUB: asm_op = "SUB"; break;
                    case TAC_MUL: asm_op = "MUL"; break;
                    case TAC_DIV: asm_op = "DIV"; break;
                    case TAC_LT:  asm_op = "SETLT"; break;
                    case TAC_GT:  asm_op = "SETGT"; break;
                    case TAC_EQ:  asm_op = "SETEQ"; break;
                    case TAC_NE:  asm_op = "SETNE"; break;
                    case TAC_AND: asm_op = "SETAND"; break;
                    case TAC_OR:  asm_op = "SETOR"; break;
                    default: break;
                }
                
                sprintf(line, "%s%s    %s R2, R0, R1\n    STORE %s, R2\n", op_part, op_part2, asm_op, inst->dest);
                break;
            }
                
            case TAC_NEG:
                load_op(op_part, "R0", inst->arg1);
                sprintf(line, "%s    NEG R1, R0\n    STORE %s, R1\n", op_part, inst->dest);
                break;
                
            case TAC_NOT:
                load_op(op_part, "R0", inst->arg1);
                sprintf(line, "%s    NOT R1, R0\n    STORE %s, R1\n", op_part, inst->dest);
                break;
                
            case TAC_LABEL:
                sprintf(line, "%s:\n", inst->dest);
                break;
                
            case TAC_IF_FALSE:
                load_op(op_part, "R0", inst->arg1);
                sprintf(line, "%s    JZ R0, %s\n", op_part, inst->dest);
                break;
                
            case TAC_GOTO:
                sprintf(line, "    JMP %s\n", inst->dest);
                break;
                
            case TAC_PRINT:
                load_op(op_part, "R0", inst->arg1);
                sprintf(line, "%s    OUT R0\n", op_part);
                break;
        }
        
        int len = (int)strlen(line);
        if (size + len + 1 > capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }
        strcat(buffer, line);
        size += len;
        
        inst = inst->next;
    }
    
    // Halt call at end of main text
    sprintf(line, "    HALT\n");
    if (size + (int)strlen(line) + 1 > capacity) {
        capacity *= 2;
        buffer = realloc(buffer, capacity);
    }
    strcat(buffer, line);
    
    return buffer;
}
