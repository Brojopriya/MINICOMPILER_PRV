#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "semantic.h"
#include "codegen.h"
#include "optimize.h"
#include "target.h"
#include "parser.tab.h"

extern int yyparse();
extern FILE *yyin;
extern int yyline;
extern int yylex();
extern void yyrestart(FILE*);
extern char* yytext;
extern ASTNode* get_ast();

const char* token_name(int token) {
    switch (token) {
        case INT: return "INT";
        case BOOL: return "BOOL";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case PRINT: return "PRINT";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case LT: return "LT";
        case GT: return "GT";
        case EQ: return "EQ";
        case NE: return "NE";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case ASSIGN: return "ASSIGN";
        case SEMICOLON: return "SEMICOLON";
        case LBRACE: return "LBRACE";
        case RBRACE: return "RBRACE";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case INT_CONST: return "INT_CONST";
        case BOOL_CONST: return "BOOL_CONST";
        case ID: return "ID";
        default: return "UNKNOWN";
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.ml>\n", argv[0]);
        return 1;
    }
    
    FILE *input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", argv[1]);
        return 1;
    }
    
    yyin = input;
    
    printf("========================================\n");
    printf("MiniLang Multi-Phase Compiler\n");
    printf("========================================\n\n");
    
    // ----------------------------------------------------
    // PHASE 1: Lexical Analysis Logging
    // ----------------------------------------------------
    printf("[1/6] Lexical Analysis...\n");
    FILE *lexer_log = fopen("phase_1_lexer.txt", "w");
    if (!lexer_log) {
        fprintf(stderr, "Error: Cannot create phase_1_lexer.txt\n");
        fclose(input);
        return 1;
    }
    
    fprintf(lexer_log, "========================================\n");
    fprintf(lexer_log, "MiniLang Lexer Scan Trace\n");
    fprintf(lexer_log, "========================================\n\n");
    
    yyline = 1;
    int tok;
    while ((tok = yylex()) != 0) {
        fprintf(lexer_log, "Line %3d | Token: %-12s | Lexeme: '%s'\n", yyline, token_name(tok), yytext);
        if (tok == ID) {
            // Free the duplicated string allocated in lexer.l to prevent memory leaks during pre-scan
            extern YYSTYPE yylval;
            if (yylval.sval) {
                free(yylval.sval);
                yylval.sval = NULL;
            }
        }
    }
    fclose(lexer_log);
    printf("        -> Token trace written to: phase_1_lexer.txt\n\n");
    
    // Reset file stream and Flex scanner state for parser
    fseek(input, 0, SEEK_SET);
    yyrestart(input);
    yyline = 1;
    
    // ----------------------------------------------------
    // PHASE 2: Syntax Analysis & AST Generation
    // ----------------------------------------------------
    printf("[2/6] Syntax Analysis...\n");
    ASTNode *ast = NULL;
    int parse_result = yyparse();
    if (parse_result == 0) {
        ast = get_ast();
        printf("        -> Parsing successful!\n");
        
        FILE *ast_log = fopen("phase_2_parser.txt", "w");
        if (ast_log) {
            fprintf(ast_log, "========================================\n");
            fprintf(ast_log, "MiniLang Abstract Syntax Tree (AST) Dump\n");
            fprintf(ast_log, "========================================\n\n");
            ast_print(ast, ast_log, 0);
            fclose(ast_log);
            printf("        -> AST written to: phase_2_parser.txt\n\n");
        } else {
            fprintf(stderr, "Error: Cannot write phase_2_parser.txt\n");
        }
    } else {
        printf("        -> Parsing failed with code: %d\n", parse_result);
        fclose(input);
        return 1;
    }
    
    fclose(input);
    
    if (!ast) {
        printf("Error: No AST generated!\n");
        return 1;
    }
    
    // ----------------------------------------------------
    // PHASE 3: Semantic Analysis
    // ----------------------------------------------------
    printf("[3/6] Semantic Analysis...\n");
    SemanticAnalyzer *semantic = semantic_analyzer_create();
    
    if (semantic_analyze(ast, semantic)) {
        printf("        -> Semantic analysis successful!\n");
        printf("        -> Semantic trace and symbols written to: phase_3_semantic.txt\n\n");
    } else {
        printf("        -> Semantic analysis completed with %d errors\n", semantic->error_count);
        printf("        -> Check phase_3_semantic.txt for error details\n\n");
        semantic_analyzer_destroy(semantic);
        ast_free(ast);
        return 1;
    }
    semantic_analyzer_destroy(semantic);
    
    // ----------------------------------------------------
    // PHASE 4: Intermediate Code Generation (Raw TAC)
    // ----------------------------------------------------
    printf("[4/6] Intermediate Code Gen (Raw TAC)...\n");
    CodeGenerator *codegen = codegen_create();
    codegen_generate(ast, codegen);
    
    FILE *raw_tac_file = fopen("phase_4_raw_tac.tac", "w");
    if (raw_tac_file) {
        fprintf(raw_tac_file, "%s", codegen_get_code(codegen));
        fclose(raw_tac_file);
        printf("        -> Raw TAC written to: phase_4_raw_tac.tac\n\n");
    } else {
        fprintf(stderr, "Error: Cannot create raw TAC file\n");
    }
    
    // ----------------------------------------------------
    // PHASE 5: Code Optimization (Optimized TAC)
    // ----------------------------------------------------
    printf("[5/6] Code Optimization...\n");
    codegen_optimize(codegen);
    printf("        -> TAC optimization successful!\n");
    
    FILE *opt_tac_file = fopen("output.tac", "w");
    if (opt_tac_file) {
        fprintf(opt_tac_file, "%s", codegen_get_code(codegen));
        fclose(opt_tac_file);
        printf("        -> Optimized TAC written to: output.tac\n\n");
    } else {
        fprintf(stderr, "Error: Cannot create output.tac\n");
        codegen_destroy(codegen);
        ast_free(ast);
        return 1;
    }
    
    // ----------------------------------------------------
    // PHASE 6: Target Code Generation (Assembly)
    // ----------------------------------------------------
    printf("[6/6] Target Code Generation (Assembly)...\n");
    char *assembly = target_generate_assembly(codegen);
    if (assembly) {
        FILE *assembly_file = fopen("output.asm", "w");
        if (assembly_file) {
            fprintf(assembly_file, "%s", assembly);
            fclose(assembly_file);
            printf("        -> Target Assembly written to: output.asm\n\n");
        } else {
            fprintf(stderr, "Error: Cannot create output.asm\n");
        }
        free(assembly);
    } else {
        printf("        -> Assembly generation failed!\n\n");
    }
    
    printf("========================================\n");
    printf("Compilation completed successfully!\n");
    printf("========================================\n");
    
    codegen_destroy(codegen);
    ast_free(ast);
    
    return 0;
}
