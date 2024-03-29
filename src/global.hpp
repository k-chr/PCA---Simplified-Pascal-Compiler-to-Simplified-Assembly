#include "compiler.hpp"

//lexer
int yylex();
int yylex_destroy();

//parser
int yyparse();
extern void yyerror(char const* s);
extern void yyerror(const std::exception& e);

extern int lineno;
extern std::shared_ptr<SymTable> symtab_ptr;
extern std::shared_ptr<Emitter> emitter_ptr;
extern std::FILE* yyin;