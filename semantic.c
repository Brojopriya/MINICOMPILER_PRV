#include "semantic.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>

SymbolTable *global_symbol_table;
int semantic_errors = 0;
FILE *semantic_log = NULL;

SemanticAnalyzer* semantic_analyzer_create() {
    SemanticAnalyzer *sa = (SemanticAnalyzer *)malloc(sizeof(SemanticAnalyzer));
    sa->error_count = 0;
    sa->warning_count = 0;
    global_symbol_table = symbol_table_create();
    return sa;
}

void semantic_analyzer_destroy(SemanticAnalyzer *sa) {
    symbol_table_destroy(global_symbol_table);
    free(sa);
}

DataType infer_type(ASTNode *expr);

void check_declarations(ASTNode *decl) {
    if (!decl) return;
    
    if (decl->type == AST_DECL) {
        if (!symbol_table_insert(global_symbol_table, decl->data.decl.id, decl->data.decl.type)) {
            fprintf(stderr, "Error: Variable '%s' already declared in current scope\n", decl->data.decl.id);
            if (semantic_log) {
                fprintf(semantic_log, "[ERROR] Duplicate declaration of variable '%s' in scope %d\n", 
                        decl->data.decl.id, global_symbol_table->scope_level);
            }
            semantic_errors++;
        } else {
            if (semantic_log) {
                fprintf(semantic_log, "[DECL] Declared variable '%s' with type '%s' in scope %d\n",
                        decl->data.decl.id,
                        (decl->data.decl.type == TYPE_INT) ? "int" : "bool",
                        global_symbol_table->scope_level);
            }
        }
        check_declarations(decl->data.decl.next);
    }
}

DataType infer_type(ASTNode *expr) {
    if (!expr) return TYPE_INT;
    
    switch (expr->type) {
        case AST_EXPR_INT:
            if (semantic_log) {
                fprintf(semantic_log, "[EXPR] Integer literal constant: %d\n", expr->data.int_val.value);
            }
            return TYPE_INT;
        case AST_EXPR_BOOL:
            if (semantic_log) {
                fprintf(semantic_log, "[EXPR] Boolean literal constant: %s\n", expr->data.bool_val.value ? "true" : "false");
            }
            return TYPE_BOOL;
        case AST_EXPR_ID: {
            SymbolEntry *entry = symbol_table_lookup(global_symbol_table, expr->data.id.name);
            if (!entry) {
                fprintf(stderr, "Error: Undeclared variable '%s'\n", expr->data.id.name);
                if (semantic_log) {
                    fprintf(semantic_log, "[ERROR] Use of undeclared variable '%s'\n", expr->data.id.name);
                }
                semantic_errors++;
                return TYPE_INT;
            }
            if (semantic_log) {
                fprintf(semantic_log, "[LOOKUP] Referenced variable '%s' (type: '%s', scope: %d)\n",
                        expr->data.id.name,
                        (entry->type == TYPE_INT) ? "int" : "bool",
                        entry->scope_level);
            }
            return entry->type;
        }
        case AST_EXPR_BINOP: {
            DataType left = infer_type(expr->data.binop.left);
            DataType right = infer_type(expr->data.binop.right);
            
            const char *op_str = "";
            switch (expr->data.binop.op) {
                case OP_PLUS: op_str = "+"; break;
                case OP_MINUS: op_str = "-"; break;
                case OP_MUL: op_str = "*"; break;
                case OP_DIV: op_str = "/"; break;
                case OP_LT: op_str = "<"; break;
                case OP_GT: op_str = ">"; break;
                case OP_EQ: op_str = "=="; break;
                case OP_NE: op_str = "!="; break;
                case OP_AND: op_str = "&&"; break;
                case OP_OR: op_str = "||"; break;
            }
            
            switch (expr->data.binop.op) {
                case OP_PLUS:
                case OP_MINUS:
                case OP_MUL:
                case OP_DIV:
                    if (left != TYPE_INT || right != TYPE_INT) {
                        fprintf(stderr, "Error: Arithmetic operation '%s' requires int operands\n", op_str);
                        if (semantic_log) {
                            fprintf(semantic_log, "[ERROR] Type mismatch in arithmetic '%s'. Got (%s, %s), expected (int, int)\n",
                                    op_str, (left == TYPE_INT)?"int":"bool", (right == TYPE_INT)?"int":"bool");
                        }
                        semantic_errors++;
                    } else {
                        if (semantic_log) {
                            fprintf(semantic_log, "[TYPE] Verified arithmetic operation '%s': (int, int) -> int\n", op_str);
                        }
                    }
                    return TYPE_INT;
                case OP_LT:
                case OP_GT:
                case OP_EQ:
                case OP_NE:
                    if (left != right) {
                        fprintf(stderr, "Error: Type mismatch in comparison '%s'\n", op_str);
                        if (semantic_log) {
                            fprintf(semantic_log, "[ERROR] Type mismatch in comparison '%s'. Got (%s, %s)\n",
                                    op_str, (left == TYPE_INT)?"int":"bool", (right == TYPE_INT)?"int":"bool");
                        }
                        semantic_errors++;
                    } else {
                        if (semantic_log) {
                            fprintf(semantic_log, "[TYPE] Verified relational comparison '%s': (%s, %s) -> bool\n",
                                    op_str, (left == TYPE_INT)?"int":"bool", (right == TYPE_INT)?"int":"bool");
                        }
                    }
                    return TYPE_BOOL;
                case OP_AND:
                case OP_OR:
                    if (left != TYPE_BOOL || right != TYPE_BOOL) {
                        fprintf(stderr, "Error: Logical operation '%s' requires bool operands\n", op_str);
                        if (semantic_log) {
                            fprintf(semantic_log, "[ERROR] Type mismatch in logical '%s'. Got (%s, %s), expected (bool, bool)\n",
                                    op_str, (left == TYPE_INT)?"int":"bool", (right == TYPE_INT)?"int":"bool");
                        }
                        semantic_errors++;
                    } else {
                        if (semantic_log) {
                            fprintf(semantic_log, "[TYPE] Verified logical operation '%s': (bool, bool) -> bool\n", op_str);
                        }
                    }
                    return TYPE_BOOL;
                default:
                    return TYPE_INT;
            }
        }
        case AST_EXPR_UNOP: {
            DataType operand_type = infer_type(expr->data.unop.operand);
            if (expr->data.unop.op == OP_NOT) {
                if (operand_type != TYPE_BOOL) {
                    fprintf(stderr, "Error: Logical negation '!' requires bool operand\n");
                    if (semantic_log) {
                        fprintf(semantic_log, "[ERROR] Logical negation '!' applied to '%s', expected bool\n",
                                (operand_type == TYPE_INT)?"int":"bool");
                    }
                    semantic_errors++;
                } else {
                    if (semantic_log) {
                        fprintf(semantic_log, "[TYPE] Verified unary logical negation '!': bool -> bool\n");
                    }
                }
                return TYPE_BOOL;
            } else if (expr->data.unop.op == OP_NEG) {
                if (operand_type != TYPE_INT) {
                    fprintf(stderr, "Error: Unary negation '-' requires int operand\n");
                    if (semantic_log) {
                        fprintf(semantic_log, "[ERROR] Unary arithmetic negation '-' applied to '%s', expected int\n",
                                (operand_type == TYPE_INT)?"int":"bool");
                    }
                    semantic_errors++;
                } else {
                    if (semantic_log) {
                        fprintf(semantic_log, "[TYPE] Verified unary arithmetic negation '-': int -> int\n");
                    }
                }
                return TYPE_INT;
            }
            return TYPE_INT;
        }
        default:
            return TYPE_INT;
    }
}

void check_statements(ASTNode *stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_STMT_LIST:
            check_statements(stmt->data.stmt_list.stmts);
            check_statements(stmt->data.stmt_list.next);
            break;
            
        case AST_STMT_BLOCK:
            if (semantic_log) {
                fprintf(semantic_log, "[SCOPE] Entering block scope. Level increases from %d -> %d\n",
                        global_symbol_table->scope_level, global_symbol_table->scope_level + 1);
            }
            symbol_table_enter_scope(global_symbol_table);
            check_statements(stmt->data.stmt_block.statements);
            if (semantic_log) {
                fprintf(semantic_log, "[SCOPE] Exiting block scope. Level decreases from %d -> %d\n",
                        global_symbol_table->scope_level, global_symbol_table->scope_level - 1);
            }
            symbol_table_exit_scope(global_symbol_table);
            break;
            
        case AST_ASSIGN: {
            SymbolEntry *entry = symbol_table_lookup(global_symbol_table, stmt->data.assign.id);
            if (!entry) {
                fprintf(stderr, "Error: Undeclared variable '%s'\n", stmt->data.assign.id);
                if (semantic_log) {
                    fprintf(semantic_log, "[ERROR] Undeclared variable '%s' in assignment LHS\n", stmt->data.assign.id);
                }
                semantic_errors++;
                break;
            }
            DataType expr_type = infer_type(stmt->data.assign.expr);
            if (entry->type != expr_type) {
                fprintf(stderr, "Error: Type mismatch in assignment to '%s'\n", stmt->data.assign.id);
                if (semantic_log) {
                    fprintf(semantic_log, "[ERROR] Type mismatch in assignment to '%s'. Variable is '%s', RHS evaluated to '%s'\n",
                            stmt->data.assign.id,
                            (entry->type == TYPE_INT) ? "int" : "bool",
                            (expr_type == TYPE_INT) ? "int" : "bool");
                }
                semantic_errors++;
            } else {
                if (semantic_log) {
                    fprintf(semantic_log, "[ASSIGN] Validated assignment to '%s'. Types match: '%s'\n",
                            stmt->data.assign.id, (entry->type == TYPE_INT) ? "int" : "bool");
                }
            }
            break;
        }
        
        case AST_IF: {
            DataType cond_type = infer_type(stmt->data.if_stmt.cond);
            if (cond_type != TYPE_BOOL) {
                fprintf(stderr, "Error: If condition must be bool\n");
                if (semantic_log) {
                    fprintf(semantic_log, "[ERROR] If condition must be bool. Got '%s'\n",
                            (cond_type == TYPE_INT) ? "int" : "bool");
                }
                semantic_errors++;
            } else {
                if (semantic_log) {
                    fprintf(semantic_log, "[TYPE] Verified If-statement condition type: bool\n");
                }
            }
            check_statements(stmt->data.if_stmt.if_block);
            if (stmt->data.if_stmt.else_block) {
                check_statements(stmt->data.if_stmt.else_block);
            }
            break;
        }
        
        case AST_WHILE: {
            DataType cond_type = infer_type(stmt->data.while_stmt.cond);
            if (cond_type != TYPE_BOOL) {
                fprintf(stderr, "Error: While condition must be bool\n");
                if (semantic_log) {
                    fprintf(semantic_log, "[ERROR] While condition must be bool. Got '%s'\n",
                            (cond_type == TYPE_INT) ? "int" : "bool");
                }
                semantic_errors++;
            } else {
                if (semantic_log) {
                    fprintf(semantic_log, "[TYPE] Verified While-loop condition type: bool\n");
                }
            }
            check_statements(stmt->data.while_stmt.body);
            break;
        }
        
        case AST_PRINT: {
            DataType expr_type = infer_type(stmt->data.print_stmt.expr);
            if (semantic_log) {
                fprintf(semantic_log, "[PRINT] Verified Print-statement expression of type '%s'\n",
                        (expr_type == TYPE_INT) ? "int" : "bool");
            }
            break;
        }
            
        default:
            break;
    }
}

int semantic_analyze(ASTNode *ast, SemanticAnalyzer *sa) {
    if (!ast || ast->type != AST_PROGRAM) {
        return 0;
    }
    
    semantic_log = fopen("phase_3_semantic.txt", "w");
    if (!semantic_log) {
        fprintf(stderr, "Warning: Could not create phase_3_semantic.txt log file\n");
    } else {
        fprintf(semantic_log, "========================================\n");
        fprintf(semantic_log, "MiniLang Semantic Analysis Log\n");
        fprintf(semantic_log, "========================================\n\n");
    }
    
    semantic_errors = 0;
    check_declarations(ast->data.program.decl_list);
    check_statements(ast->data.program.stmt_list);
    
    sa->error_count = semantic_errors;
    
    if (semantic_log) {
        fprintf(semantic_log, "\n========================================\n");
        fprintf(semantic_log, "Final Active Symbol Table State:\n");
        fprintf(semantic_log, "========================================\n");
        symbol_table_print(global_symbol_table, semantic_log);
        
        fprintf(semantic_log, "\n========================================\n");
        fprintf(semantic_log, "Analysis complete with %d error(s)\n", semantic_errors);
        fprintf(semantic_log, "========================================\n");
        fclose(semantic_log);
        semantic_log = NULL;
    }
    
    return semantic_errors == 0;
}
