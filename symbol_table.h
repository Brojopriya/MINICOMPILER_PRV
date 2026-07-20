#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include "ast.h"


typedef struct SymbolEntry {
    char *name;
    DataType type;
    int scope_level;
    struct SymbolEntry *next;
} SymbolEntry;

typedef struct {
    SymbolEntry *table;
    int scope_level;
} SymbolTable;

SymbolTable* symbol_table_create();
void symbol_table_destroy(SymbolTable *st);
int symbol_table_insert(SymbolTable *st, char *name, DataType type);
SymbolEntry* symbol_table_lookup(SymbolTable *st, char *name);
void symbol_table_enter_scope(SymbolTable *st);
void symbol_table_exit_scope(SymbolTable *st);
int symbol_table_lookup_in_current_scope(SymbolTable *st, char *name);
void symbol_table_print(SymbolTable *st, FILE *out);

#endif
