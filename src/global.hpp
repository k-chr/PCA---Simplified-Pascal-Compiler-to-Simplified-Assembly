#include "types/compiler.hpp"
#include <memory>

//gen/lexer
int yylex();
int yylex_destroy();

//gen/parser
int yyparse();
void yyerror(char const* s);

extern int lineno;
extern std::shared_ptr<SymTable> sym_tab_ptr;
extern std::shared_ptr<Emitter> emitter_ptr;