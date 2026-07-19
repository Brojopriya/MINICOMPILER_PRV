%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

int yylex();
void yyerror(const char *s);
extern int yyline;

ASTNode *ast_root = NULL;

%}

%union {
    int ival;
    char *sval;
    ASTNode *node;
}

%token INT BOOL IF ELSE WHILE PRINT
%token PLUS MINUS MUL DIV
%token LT GT EQ NE AND OR NOT
%token ASSIGN SEMICOLON LBRACE RBRACE LPAREN RPAREN

%token <ival> INT_CONST BOOL_CONST
%token <sval> ID

%type <node> program declarations declaration statements statement
%type <node> expression primary_expr
%type <ival> type_specifier

%left OR
%left AND
%left EQ NE
%left LT GT
%left PLUS MINUS
%left MUL DIV
%right NOT UMINUS
%nonassoc IF
%nonassoc ELSE

%%

program : declarations statements
    {
        $$ = ast_create_program($1, $2);
        ast_root = $$;
    }
    ;

declarations : declarations declaration
    {
        if ($1) {
            ASTNode *curr = $1;
            while (curr->data.decl.next) {
                curr = curr->data.decl.next;
            }
            curr->data.decl.next = $2;
            $$ = $1;
        } else {
            $$ = $2;
        }
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

declaration : type_specifier ID SEMICOLON
    {
        $$ = ast_create_decl($2, $1, NULL);
        free($2);
    }
    ;

type_specifier : INT
    {
        $$ = TYPE_INT;
    }
    | BOOL
    {
        $$ = TYPE_BOOL;
    }
    ;

statements : statements statement
    {
        if ($1) {
            ASTNode *curr = $1;
            while (curr->type == AST_STMT_LIST && curr->data.stmt_list.next) {
                curr = curr->data.stmt_list.next;
            }
            ASTNode *stmt_list = ast_create_stmt_list($2, NULL);
            if (curr->type == AST_STMT_LIST) {
                curr->data.stmt_list.next = stmt_list;
            }
            $$ = $1;
        } else {
            $$ = ast_create_stmt_list($2, NULL);
        }
    }
    | /* empty */
    {
        $$ = NULL;
    }
    ;

statement : ID ASSIGN expression SEMICOLON
    {
        $$ = ast_create_assign($1, $3);
        free($1);
    }
    | IF LPAREN expression RPAREN statement ELSE statement
    {
        $$ = ast_create_if($3, $5, $7);
    }
    | IF LPAREN expression RPAREN statement %prec IF
    {
        $$ = ast_create_if($3, $5, NULL);
    }
    | WHILE LPAREN expression RPAREN statement
    {
        $$ = ast_create_while($3, $5);
    }
    | PRINT LPAREN expression RPAREN SEMICOLON
    {
        $$ = ast_create_print($3);
    }
    | LBRACE statements RBRACE
    {
        $$ = ast_create_stmt_block($2);
    }
    ;

expression : expression PLUS expression
    {
        $$ = ast_create_binop(OP_PLUS, $1, $3);
    }
    | expression MINUS expression
    {
        $$ = ast_create_binop(OP_MINUS, $1, $3);
    }
    | expression MUL expression
    {
        $$ = ast_create_binop(OP_MUL, $1, $3);
    }
    | expression DIV expression
    {
        $$ = ast_create_binop(OP_DIV, $1, $3);
    }
    | expression LT expression
    {
        $$ = ast_create_binop(OP_LT, $1, $3);
    }
    | expression GT expression
    {
        $$ = ast_create_binop(OP_GT, $1, $3);
    }
    | expression EQ expression
    {
        $$ = ast_create_binop(OP_EQ, $1, $3);
    }
    | expression NE expression
    {
        $$ = ast_create_binop(OP_NE, $1, $3);
    }
    | expression AND expression
    {
        $$ = ast_create_binop(OP_AND, $1, $3);
    }
    | expression OR expression
    {
        $$ = ast_create_binop(OP_OR, $1, $3);
    }
    | NOT expression
    {
        $$ = ast_create_unop(OP_NOT, $2);
    }
    | MINUS expression %prec UMINUS
    {
        $$ = ast_create_unop(OP_NEG, $2);
    }
    | primary_expr
    {
        $$ = $1;
    }
    ;

primary_expr : INT_CONST
    {
        $$ = ast_create_int($1);
    }
    | BOOL_CONST
    {
        $$ = ast_create_bool($1);
    }
    | ID
    {
        $$ = ast_create_id($1);
        free($1);
    }
    | LPAREN expression RPAREN
    {
        $$ = $2;
    }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax error at line %d: %s\n", yyline, s);
}

ASTNode* get_ast() {
    return ast_root;
}
