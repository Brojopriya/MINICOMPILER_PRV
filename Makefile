CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -Wall -Wextra -g

TARGET = minicompiler
SOURCES = main.c ast.c symbol_table.c semantic.c codegen.c optimize.c target.c lex.yy.c parser.tab.c
HEADERS = ast.h symbol_table.h semantic.h codegen.h optimize.h target.h

all: $(TARGET)

$(TARGET): lex.yy.c parser.tab.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

lex.yy.c: lexer.l parser.tab.h
	$(FLEX) lexer.l

parser.tab.c parser.tab.h: parser.y
	$(BISON) -d parser.y

clean:
	rm -f $(TARGET) lex.yy.c parser.tab.c parser.tab.h output.tac output.asm phase_*.txt phase_*.tac

run: $(TARGET)
	./$(TARGET) test_simple.ml

.PHONY: all clean run
