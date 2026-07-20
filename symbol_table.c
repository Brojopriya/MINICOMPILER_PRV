#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>

SymbolTable* symbol_table_create() {
    SymbolTable *st = (SymbolTable *)malloc(sizeof(SymbolTable));
    st->table = NULL;
    st->scope_level = 0;
    return st;
}

void symbol_table_destroy(SymbolTable *st) {
    SymbolEntry *entry = st->table;
    while (entry) {
        SymbolEntry *temp = entry;
        entry = entry->next;
        free(temp->name);
        free(temp);
    }
    free(st);
}

int symbol_table_insert(SymbolTable *st, char *name, DataType type) {
    if (symbol_table_lookup_in_current_scope(st, name)) {
        return 0;
    }
    
    SymbolEntry *entry = (SymbolEntry *)malloc(sizeof(SymbolEntry));
    entry->name = strdup(name);
    entry->type = type;
    entry->scope_level = st->scope_level;
    entry->next = st->table;
    st->table = entry;
    return 1;
}

SymbolEntry* symbol_table_lookup(SymbolTable *st, char *name) {
    SymbolEntry *entry = st->table;
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

int symbol_table_lookup_in_current_scope(SymbolTable *st, char *name) {
    SymbolEntry *entry = st->table;
    while (entry) {
        if (strcmp(entry->name, name) == 0 && entry->scope_level == st->scope_level) {
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}

void symbol_table_enter_scope(SymbolTable *st) {
    st->scope_level++;
}

void symbol_table_exit_scope(SymbolTable *st) {
    SymbolEntry *entry = st->table;
    SymbolEntry *prev = NULL;
    while (entry) {
        if (entry->scope_level == st->scope_level) {
            if (prev) {
                prev->next = entry->next;
            } else {
                st->table = entry->next;
            }
            SymbolEntry *temp = entry;
            entry = entry->next;
            free(temp->name);
            free(temp);
        } else {
            prev = entry;
            entry = entry->next;
        }
    }
    st->scope_level--;
}

void symbol_table_print(SymbolTable *st, FILE *out) {
    if (!out || !st) return;
    
    fprintf(out, "+------------------+------------+-------------+\n");
    fprintf(out, "| Identifier Name  | Data Type  | Scope Level |\n");
    fprintf(out, "+------------------+------------+-------------+\n");
    
    SymbolEntry *entry = st->table;
    if (!entry) {
        fprintf(out, "| <empty>          | -          | -           |\n");
    }
    
    while (entry) {
        fprintf(out, "| %-16s | %-10s | %-11d |\n",
                entry->name,
                (entry->type == TYPE_INT) ? "int" : "bool",
                entry->scope_level);
        entry = entry->next;
    }
    fprintf(out, "+------------------+------------+-------------+\n");
}
