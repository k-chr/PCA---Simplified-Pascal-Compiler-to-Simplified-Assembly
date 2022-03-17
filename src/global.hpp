#include "compiler.hpp"
#include <exception>
#include <memory>

//gen/lexer
int yylex();
int yylex_destroy();

//gen/parser
int yyparse();
extern void yyerror(char const* s);
extern void yyerror(std::exception& e);

extern int lineno;
extern std::shared_ptr<SymTable> symtab_ptr;
extern std::shared_ptr<Emitter> emitter_ptr;
