# MiniLang Compiler - Implementation Report

<!-- ## 1. Project Overview

This document describes the complete implementation of a MiniLang compiler that performs lexical analysis, syntax analysis, semantic analysis, and generates Three-Address Code (TAC) intermediate representation. The compiler follows a classical multi-phase architecture.

## 2. Language Features Implemented

### 2.1 Data Types
- **int**: 32-bit integer type
- **bool**: Boolean type (true/false)

### 2.2 Statements
- Variable declarations
- Assignment statements
- If-else conditionals
- While loops
- Print statements
- Block scoping with braces { }

### 2.3 Expressions
- Arithmetic operators: `+`, `-`, `*`, `/`
- Relational operators: `<`, `>`, `==`, `!=`
- Logical operators: `&&`, `||`, `!`
- Unary operators: `-`, `!`
- Integer and boolean literals
- Variable identifiers -->

## 3. Compiler Phases

<!-- ### 3.1 Lexical Analysis (Flex) -->

**File**: `lexer.l`

The lexical analyzer performs:
- Tokenization of input source code
- Recognition of keywords: `int`, `bool`, `if`, `else`, `while`, `print`
- Identification of identifiers, integer constants, and operators
- Comment handling (`//` style comments)
- Whitespace and newline management
- Line number tracking for error reporting

**Regular Expression Patterns**:
```
Keywords:  int|bool|if|else|while|print|true|false
Numbers:   [0-9]+
Identifiers: [a-zA-Z_][a-zA-Z0-9_]*
Operators: +|-|*|/|<|>|==|!=|&&|||!|=
Delimiters: ;|{|}|(|)
Comments:  //.*\n
```

**Time Complexity**: O(n) where n is the length of input source
**Space Complexity**: O(1) amortized (for token storage)

### 3.2 Syntax Analysis (Bison)

**File**: `parser.y`

The parser implements a context-free grammar that:
- Validates program structure
- Resolves operator precedence and associativity
- Constructs an Abstract Syntax Tree (AST)
- Reports syntax errors with line numbers

**Grammar Structure**:
```
program     → declarations statements
declarations → declarations declaration | ε
declaration → type_specifier ID ;
type_specifier → int | bool
statements  → statements statement | ε
statement   → assignment | if-statement | while-statement | print-statement | block
expression  → binary operations | unary operations | primary expressions
```

**Operator Precedence** (from lowest to highest):
1. Logical OR (`||`)
2. Logical AND (`&&`)
3. Equality (`==`, `!=`)
4. Relational (`<`, `>`)
5. Additive (`+`, `-`)
6. Multiplicative (`*`, `/`)
7. Unary (`-`, `!`)

**Shift-Reduce Conflicts**: None (LALR(1) grammar)

**Time Complexity**: O(n) for LALR(1) parsing
**Space Complexity**: O(n) for parse stack depth

### 3.3 Abstract Syntax Tree (AST)

**Files**: `ast.h`, `ast.c`

The AST is the primary intermediate representation before code generation.

**Node Types**:
```c
typedef enum {
    AST_PROGRAM,      // Root node
    AST_DECL,         // Variable declaration
    AST_STMT_LIST,    // Statement sequence
    AST_STMT_BLOCK,   // Compound statement { }
    AST_ASSIGN,       // Assignment statement
    AST_IF,           // If-else statement
    AST_WHILE,        // While loop
    AST_PRINT,        // Print statement
    AST_EXPR_BINOP,   // Binary operation
    AST_EXPR_UNOP,    // Unary operation
    AST_EXPR_ID,      // Variable reference
    AST_EXPR_INT,     // Integer literal
    AST_EXPR_BOOL     // Boolean literal
}
```

**Memory Usage**: Each node stores type, data type, and a union of operation-specific data
**Construction Time**: O(1) per node
**Total AST Construction**: O(n) where n is number of nodes

 ### 3.4 Semantic Analysis

**Files**: `semantic.h`, `semantic.c`, `symbol_table.h`, `symbol_table.c`

The semantic analysis phase performs symbol table management and static type checking by traversing the Abstract Syntax Tree (AST).

#### Symbol Table

The symbol table is implemented using a linked list and maintains scope information for every declared identifier.

**Features**
- Stores identifier name, data type, and scope level.
- Prevents duplicate declarations within the same scope.
- Supports identifier lookup.
- Supports nested scopes using enter and exit scope operations.
- Prints the active symbol table for debugging.

#### Semantic Analysis

The semantic analyzer performs the following checks:

- Duplicate variable declaration detection.
- Undeclared variable usage detection.
- Assignment type checking.
- Arithmetic expression type checking.
- Relational expression type checking.
- Logical expression type checking.
- Validation of `if` conditions.
- Validation of `while` conditions.

#### Error Reporting

The implementation reports:
- Duplicate declarations.
- Undeclared identifiers.
- Type mismatch in assignments.
- Invalid operand types.
- Invalid control-statement conditions.

A semantic log (`phase_3_semantic.txt`) is generated containing declaration information, scope transitions, symbol table contents, and semantic errors.

**Time Complexity**
- Symbol insertion: O(1)
- Symbol lookup: O(s)
- Overall semantic analysis: O(n × s)

**Space Complexity**
- O(s), where `s` is the number of active symbols.
<!--
### 3.5 Intermediate Code Generation (TAC)

**Files**: `codegen.h`, `codegen.c`

Generates Three-Address Code from the AST.

**TAC Format**:
```
temp = operand1 op operand2    // Binary operations
temp = op operand              // Unary operations
variable = temp                // Assignment
label:                          // Jump targets
if_false temp goto label        // Conditional jump
goto label                      // Unconditional jump
print temp                      // Print operation
```

**Code Generation Strategy**:

1. **Expressions**: Recursively evaluate subexpressions, create temporaries
2. **Statements**: Generate TAC for each statement type
3. **Control Flow**:
   - `if-else`: Generate condition evaluation, two labels, conditional jump
   - `while`: Generate loop label, condition check, end label, unconditional jump

**Example Transformation**:
```
Input:  while (x < 5) { x = x + 1; }
Output:
    L0:
    t1 = x < 5
    if_false t1 goto L1
    t2 = x + 1
    x = t2
    goto L0
    L1:
```

**Temporary Generation**: `t0, t1, t2, ...` (incremental counter)
**Label Generation**: `L0, L1, L2, ...` (incremental counter)

**Time Complexity**: O(n) single AST traversal
**Space Complexity**: O(m) where m is size of generated TAC code

## 4. Overall Compiler Complexity

**Combined Time Complexity**:
- Lexical Analysis: O(n)
- Syntax Analysis: O(n)
- Semantic Analysis: O(n·s) where s is symbol count
- Code Generation: O(n)
- **Total: O(n·s)** - Linear in input size with symbol table overhead

**Combined Space Complexity**:
- AST: O(n) for parse tree nodes
- Symbol Table: O(s) for symbols
- TAC Buffer: O(m) for generated code
- **Total: O(n + s + m)**

**Linear Time Operation**: The compiler operates in linear time with respect to input size (with bounded symbol table). For typical programs with reasonable symbol counts, s << n, making it effectively linear.

## 5. Project Structure

```
MiniCompiler/
├── lexer.l              # Flex lexical analyzer
├── parser.y             # Bison grammar
├── ast.h/ast.c          # AST definitions and construction
├── symbol_table.h/c     # Symbol table implementation
├── semantic.h/c         # Semantic analyzer
├── codegen.h/c          # TAC code generator
├── main.c               # Compiler driver
├── Makefile             # Build automation
└── testcases/
    ├── test_simple.ml   # Basic arithmetic
    ├── test_if.ml       # Conditional statements
    ├── test_while.ml    # Loop statements
    ├── test_scope.ml    # Block scoping
    └── test_complex.ml  # Complex expressions
```

## 6. Compilation Instructions

### Prerequisites
- GCC compiler
- Flex (lexical analyzer generator)
- Bison (parser generator)

### Build
```bash
make
```

This will:
1. Run Flex to generate `lex.yy.c`
2. Run Bison to generate `parser.tab.c` and `parser.tab.h`
3. Compile all sources to produce `minicompiler` executable

### Clean
```bash
make clean
```

### Run Example
```bash
./minicompiler testcases/test_simple.ml
```

Output will be written to `output.tac`

## 7. Sample Compilation Results

### Example 1: Simple Arithmetic

**Input** (`test_simple.ml`):
```
int x;
int y;
int z;

x = 5;
y = 10;
z = x + y;
print(z);
```

**Output** (`output.tac`):
```
; Three-Address Code (TAC) Generated
; Variable declarations:
; int x
; int y
; int z

; Statements:
t0 = 5
x = t0
t1 = 10
y = t1
t2 = x + y
z = t2
t3 = z
print t3
```

### Example 2: Control Flow

**Input** (`test_if.ml`):
```
int num;
bool flag;

num = 15;
flag = true;

if (num > 10) {
    print(100);
} else {
    print(50);
}
```

**Output** (`output.tac`):
```
; Three-Address Code (TAC) Generated
; Variable declarations:
; int num
; bool flag

; Statements:
t0 = 15
num = t0
t1 = 1
flag = t1
t2 = num > 10
if_false t2 goto L0
t3 = 100
print t3
goto L1
L0:
t4 = 50
print t4
L1:
```

### Example 3: Loops

**Input** (`test_while.ml`):
```
int counter;
int sum;

counter = 0;
sum = 0;

while (counter < 5) {
    sum = sum + counter;
    counter = counter + 1;
}

print(sum);
```

**Output** (`output.tac`):
```
; Three-Address Code (TAC) Generated
; Variable declarations:
; int counter
; int sum

; Statements:
t0 = 0
counter = t0
t1 = 0
sum = t1
L0:
t2 = counter < 5
if_false t2 goto L1
t3 = sum + counter
sum = t3
t4 = counter + 1
counter = t4
goto L0
L1:
t5 = sum
print t5
```

## 8. Key Design Decisions

1. **AST over Parse Tree**: AST is more compact and semantically meaningful than parse trees
2. **Linked List Symbol Table**: Simple, efficient for typical programs
3. **Single-pass Semantic Analysis**: Combined with TAC generation
4. **TAC Format**: Simplified but complete representation of control flow
5. **Temporary Naming**: Sequential naming (t0, t1, ...) for clarity

## 9. Testing Approach

The provided test cases cover:
- Basic declarations and assignments
- Arithmetic operations
- Boolean operations and conditions
- If-else statements
- While loops
- Block scoping
- Complex nested expressions

## 10. Limitations and Future Work

**Current Limitations**:
- No function definitions (as per specification)
- No array support (as per specification)
- No floating-point arithmetic
- No optimization phase (specified for later implementation)
- No target code generation (specified for later implementation)

**Future Enhancement**:
- Code optimization passes (constant folding, dead code elimination)
- Target code generation to pseudo-assembly
- Extended type system
- Function definitions and calls -->
