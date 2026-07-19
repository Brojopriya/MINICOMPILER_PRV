#include "ast.h"
#include <stdlib.h>
#include <string.h>

ASTNode* ast_create_node(NodeType type) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = type;
    node->data_type = TYPE_INT;
    node->line_no = 0;
    return node;
}

ASTNode* ast_create_program(ASTNode *decls, ASTNode *stmts) {
    ASTNode *node = ast_create_node(AST_PROGRAM);
    node->data.program.decl_list = decls;
    node->data.program.stmt_list = stmts;
    return node;
}

ASTNode* ast_create_decl(char *id, DataType type, ASTNode *next) {
    ASTNode *node = ast_create_node(AST_DECL);
    node->data.decl.id = strdup(id);
    node->data.decl.type = type;
    node->data.decl.next = next;
    node->data_type = type;
    return node;
}

ASTNode* ast_create_stmt_list(ASTNode *stmt, ASTNode *next) {
    ASTNode *node = ast_create_node(AST_STMT_LIST);
    node->data.stmt_list.stmts = stmt;
    node->data.stmt_list.next = next;
    return node;
}

ASTNode* ast_create_stmt_block(ASTNode *stmts) {
    ASTNode *node = ast_create_node(AST_STMT_BLOCK);
    node->data.stmt_block.statements = stmts;
    return node;
}

ASTNode* ast_create_assign(char *id, ASTNode *expr) {
    ASTNode *node = ast_create_node(AST_ASSIGN);
    node->data.assign.id = strdup(id);
    node->data.assign.expr = expr;
    return node;
}

ASTNode* ast_create_if(ASTNode *cond, ASTNode *if_block, ASTNode *else_block) {
    ASTNode *node = ast_create_node(AST_IF);
    node->data.if_stmt.cond = cond;
    node->data.if_stmt.if_block = if_block;
    node->data.if_stmt.else_block = else_block;
    return node;
}

ASTNode* ast_create_while(ASTNode *cond, ASTNode *body) {
    ASTNode *node = ast_create_node(AST_WHILE);
    node->data.while_stmt.cond = cond;
    node->data.while_stmt.body = body;
    return node;
}

ASTNode* ast_create_print(ASTNode *expr) {
    ASTNode *node = ast_create_node(AST_PRINT);
    node->data.print_stmt.expr = expr;
    return node;
}

ASTNode* ast_create_binop(BinOp op, ASTNode *left, ASTNode *right) {
    ASTNode *node = ast_create_node(AST_EXPR_BINOP);
    node->data.binop.op = op;
    node->data.binop.left = left;
    node->data.binop.right = right;
    return node;
}

ASTNode* ast_create_unop(UnOp op, ASTNode *operand) {
    ASTNode *node = ast_create_node(AST_EXPR_UNOP);
    node->data.unop.op = op;
    node->data.unop.operand = operand;
    return node;
}

ASTNode* ast_create_id(char *name) {
    ASTNode *node = ast_create_node(AST_EXPR_ID);
    node->data.id.name = strdup(name);
    return node;
}

ASTNode* ast_create_int(int value) {
    ASTNode *node = ast_create_node(AST_EXPR_INT);
    node->data.int_val.value = value;
    node->data_type = TYPE_INT;
    return node;
}

ASTNode* ast_create_bool(int value) {
    ASTNode *node = ast_create_node(AST_EXPR_BOOL);
    node->data.bool_val.value = value;
    node->data_type = TYPE_BOOL;
    return node;
}

void ast_free(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            ast_free(node->data.program.decl_list);
            ast_free(node->data.program.stmt_list);
            break;
        case AST_DECL:
            free(node->data.decl.id);
            ast_free(node->data.decl.next);
            break;
        case AST_STMT_LIST:
            ast_free(node->data.stmt_list.stmts);
            ast_free(node->data.stmt_list.next);
            break;
        case AST_STMT_BLOCK:
            ast_free(node->data.stmt_block.statements);
            break;
        case AST_ASSIGN:
            free(node->data.assign.id);
            ast_free(node->data.assign.expr);
            break;
        case AST_IF:
            ast_free(node->data.if_stmt.cond);
            ast_free(node->data.if_stmt.if_block);
            ast_free(node->data.if_stmt.else_block);
            break;
        case AST_WHILE:
            ast_free(node->data.while_stmt.cond);
            ast_free(node->data.while_stmt.body);
            break;
        case AST_PRINT:
            ast_free(node->data.print_stmt.expr);
            break;
        case AST_EXPR_BINOP:
            ast_free(node->data.binop.left);
            ast_free(node->data.binop.right);
            break;
        case AST_EXPR_UNOP:
            ast_free(node->data.unop.operand);
            break;
        case AST_EXPR_ID:
            free(node->data.id.name);
            break;
        default:
            break;
    }
    free(node);
}
