#include "optimize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_constant(const char *str) {
    if (!str) return 0;
    int i = 0;
    if (str[0] == '-' && str[1] != '\0') i = 1; // negative numbers
    for (; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') return 0;
    }
    return 1;
}

static void delete_instruction(CodeGenerator *cg, TACInst *inst) {
    if (!inst) return;
    
    if (inst->prev) {
        inst->prev->next = inst->next;
    } else {
        cg->head = inst->next;
    }
    
    if (inst->next) {
        inst->next->prev = inst->prev;
    } else {
        cg->tail = inst->prev;
    }
    
    if (inst->dest) free(inst->dest);
    if (inst->arg1) free(inst->arg1);
    if (inst->arg2) free(inst->arg2);
    free(inst);
}

static int is_temp(const char *str) {
    return str && str[0] == 't' && str[1] >= '0' && str[1] <= '9';
}

static int run_constant_folding(CodeGenerator *cg) {
    int changed = 0;
    TACInst *inst = cg->head;
    while (inst) {
        TACInst *next = inst->next;
        
        // Unary folding
        if (inst->op == TAC_NEG && is_constant(inst->arg1)) {
            int val = atoi(inst->arg1);
            char res_str[32];
            sprintf(res_str, "%d", -val);
            
            inst->op = TAC_ASSIGN;
            free(inst->arg1);
            inst->arg1 = strdup(res_str);
            changed = 1;
        }
        else if (inst->op == TAC_NOT && is_constant(inst->arg1)) {
            int val = atoi(inst->arg1);
            char res_str[32];
            sprintf(res_str, "%d", !val);
            
            inst->op = TAC_ASSIGN;
            free(inst->arg1);
            inst->arg1 = strdup(res_str);
            changed = 1;
        }
        // Binary folding
        else if (inst->arg1 && inst->arg2 && is_constant(inst->arg1) && is_constant(inst->arg2)) {
            int val1 = atoi(inst->arg1);
            int val2 = atoi(inst->arg2);
            int res_val = 0;
            int valid = 1;
            
            switch (inst->op) {
                case TAC_ADD: res_val = val1 + val2; break;
                case TAC_SUB: res_val = val1 - val2; break;
                case TAC_MUL: res_val = val1 * val2; break;
                case TAC_DIV: 
                    if (val2 != 0) {
                        res_val = val1 / val2; 
                    } else {
                        valid = 0; // Don't fold division by zero
                    }
                    break;
                case TAC_LT: res_val = val1 < val2; break;
                case TAC_GT: res_val = val1 > val2; break;
                case TAC_EQ: res_val = val1 == val2; break;
                case TAC_NE: res_val = val1 != val2; break;
                case TAC_AND: res_val = val1 && val2; break;
                case TAC_OR: res_val = val1 || val2; break;
                default: valid = 0; break;
            }
            
            if (valid) {
                char res_str[32];
                sprintf(res_str, "%d", res_val);
                
                inst->op = TAC_ASSIGN;
                free(inst->arg1);
                free(inst->arg2);
                inst->arg1 = strdup(res_str);
                inst->arg2 = NULL;
                changed = 1;
            }
        }
        inst = next;
    }
    return changed;
}

static int run_constant_propagation(CodeGenerator *cg) {
    int changed = 0;
    TACInst *inst = cg->head;
    while (inst) {
        TACInst *next = inst->next;
        
        // If we see temp = constant
        if (inst->op == TAC_ASSIGN && is_temp(inst->dest) && is_constant(inst->arg1)) {
            char *temp_name = inst->dest;
            char *const_val = inst->arg1;
            
            // Search forward for occurrences
            TACInst *scan = inst->next;
            int count_replacements = 0;
            while (scan) {
                if (scan->arg1 && strcmp(scan->arg1, temp_name) == 0) {
                    free(scan->arg1);
                    scan->arg1 = strdup(const_val);
                    count_replacements++;
                    changed = 1;
                }
                if (scan->arg2 && strcmp(scan->arg2, temp_name) == 0) {
                    free(scan->arg2);
                    scan->arg2 = strdup(const_val);
                    count_replacements++;
                    changed = 1;
                }
                // If it is reassigned, stop (though temps in SSA are assigned once)
                if (scan->dest && strcmp(scan->dest, temp_name) == 0) {
                    break;
                }
                scan = scan->next;
            }
            
            // Since temp is fully propagated, we can delete its definition assignment
            delete_instruction(cg, inst);
            changed = 1;
            inst = next;
            continue;
        }
        inst = next;
    }
    return changed;
}

static int run_redundant_temporary_elimination(CodeGenerator *cg) {
    int changed = 0;
    TACInst *inst = cg->head;
    while (inst) {
        TACInst *next = inst->next;
        
        // Look for copy assignment: var = tX where var is NOT a temporary and tX is a temporary
        if (inst->op == TAC_ASSIGN && !is_temp(inst->dest) && is_temp(inst->arg1)) {
            char *var_name = inst->dest;
            char *temp_name = inst->arg1;
            
            // Look backward for the instruction defining temp_name
            TACInst *prev_inst = inst->prev;
            while (prev_inst) {
                // If we find the instruction defining temp_name
                if (prev_inst->dest && strcmp(prev_inst->dest, temp_name) == 0) {
                    // Check if temp_name is used anywhere else. If not, we can safely merge them!
                    // Since it's a compiler-generated temp, it is only used once in the copy assignment.
                    
                    // Rewrite its destination directly to var_name!
                    free(prev_inst->dest);
                    prev_inst->dest = strdup(var_name);
                    
                    // Delete the copy assignment inst (var = temp_name)
                    delete_instruction(cg, inst);
                    changed = 1;
                    break;
                }
                
                // If we encounter a label or control-flow jump, do not cross it to be safe
                if (prev_inst->op == TAC_LABEL || prev_inst->op == TAC_IF_FALSE || prev_inst->op == TAC_GOTO) {
                    break;
                }
                
                prev_inst = prev_inst->prev;
            }
            if (changed) {
                inst = next;
                continue;
            }
        }
        inst = next;
    }
    return changed;
}

static int run_algebraic_simplifications(CodeGenerator *cg) {
    int changed = 0;
    TACInst *inst = cg->head;
    while (inst) {
        TACInst *next = inst->next;
        
        // Additive Identity: x + 0 -> x
        if (inst->op == TAC_ADD && inst->arg2 && strcmp(inst->arg2, "0") == 0) {
            inst->op = TAC_ASSIGN;
            free(inst->arg2);
            inst->arg2 = NULL;
            changed = 1;
        }
        // Subtractive Identity: x - 0 -> x
        else if (inst->op == TAC_SUB && inst->arg2 && strcmp(inst->arg2, "0") == 0) {
            inst->op = TAC_ASSIGN;
            free(inst->arg2);
            inst->arg2 = NULL;
            changed = 1;
        }
        // Multiplicative Identity: x * 1 -> x
        else if (inst->op == TAC_MUL && inst->arg2 && strcmp(inst->arg2, "1") == 0) {
            inst->op = TAC_ASSIGN;
            free(inst->arg2);
            inst->arg2 = NULL;
            changed = 1;
        }
        // Multiplicative Zero: x * 0 -> 0
        else if (inst->op == TAC_MUL && inst->arg2 && strcmp(inst->arg2, "0") == 0) {
            inst->op = TAC_ASSIGN;
            free(inst->arg1);
            inst->arg1 = strdup("0");
            free(inst->arg2);
            inst->arg2 = NULL;
            changed = 1;
        }
        
        inst = next;
    }
    return changed;
}

void codegen_optimize(CodeGenerator *cg) {
    if (!cg) return;
    
    int changed = 1;
    int iterations = 0;
    // Limit to 10 iterations to prevent infinite loops in weird edge cases
    while (changed && iterations < 10) {
        changed = 0;
        
        // 1. Fold constants (e.g. t0 = 5 + 10 -> t0 = 15)
        changed |= run_constant_folding(cg);
        
        // 2. Propagate constants and remove unused temp assignments (e.g. t0 = 15; x = t0 -> x = 15)
        changed |= run_constant_propagation(cg);
        
        // 3. Remove redundant temporaries (e.g. t4 = x + y; z = t4 -> z = x + y)
        changed |= run_redundant_temporary_elimination(cg);
        
        // 4. Perform algebraic simplifications (e.g. x + 0 -> x)
        changed |= run_algebraic_simplifications(cg);
        
        iterations++;
    }
}
