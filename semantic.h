#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"

typedef struct {
    int error_count;
    int warning_count;
} SemanticAnalyzer;

SemanticAnalyzer* semantic_analyzer_create();
void semantic_analyzer_destroy(SemanticAnalyzer *sa);
int semantic_analyze(ASTNode *ast, SemanticAnalyzer *sa);

#endif
