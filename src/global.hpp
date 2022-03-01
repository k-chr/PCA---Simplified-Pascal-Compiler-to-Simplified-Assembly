#include "types/compiler.hpp"

//gen/lexer
int yylex();
int yylex_destroy();

//gen/parser
int yyparse();
void yyerror(char const* s);

static int lineno = 0;
static SymTable* sym_tab_ptr = nullptr;