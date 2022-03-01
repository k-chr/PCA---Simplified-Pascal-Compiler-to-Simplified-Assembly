#include "types/compiler.hpp"

typedef struct YYSTYPE YYSTYPE;
struct YYSTYPE
{
	int int_val;
	token token_val;
};
# define YYSTYPE_IS_TRIVIAL 0
# define YYSTYPE_IS_DECLARED 1

//gen/lexer
int yylex();
int yylex_destroy();

//gen/parser
int yyparse();
void yyerror(char const* s);

static int lineno = 0;