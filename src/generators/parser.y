%{
	#include "global.hpp"
%}

%output "parser.cpp"

%union	{
	int int_val;
}

%type <int_val> program header program_args variable_decl subprogram_decl block eof identifiers type
subprogram arguments statements statement_list statement args_decl arg_decl primitives array_decl range 
dims variable expression and_then or_else mulop dim_expr dim_exprs dim_comma_expr comma_expr optional_else
call inc_or_dec expression_list read write simple_expression term factor



%token <int_val> PROGRAM BEGIN_TOK END VAR INTEGER REAL ARRAY OF FUN PROC IF THEN ELSE DO WHILE REPEAT
UNTIL FOR IN TO DOWNTO WRITE READ RELOP MULOP SIGN ASSIGN AND OR NOT ID NUM NONE DONE

%%

program:
	PROGRAM ID '(' program_args ')' ';' variable_decl subprogram_decl
	{

	}
	block
	'.'
	{

	}
	eof
	;

program_args:
	ID
	| program_args ',' ID
	;

variable_decl:
	variable_decl VAR identifiers ':' type ';'
	{

	}
	| %empty
	;

identifiers:
	ID
	{

	}
	| identifiers ',' ID
	{

	}
	;

subprogram_decl:
	subprogram_decl subprogram ';'
	| %empty
	;

subprogram:
	header variable_decl block
	{

	}
	;

header:
	FUN ID
	{

	}
	arguments
	{

	}
	':' type
	{

	}
	';'
	| PROC ID
	{

	}
	arguments
	{
		
	}
	';'
	;

block:
	BEGIN_TOK
	statements
	END
	;

statements:
	statement_list
	| %empty
	;

statement_list:
	statement
	| statement_list ';' statement
	;

statement:
	variable ASSIGN expression
	{

	}
	| block
	| call
	| 	IF expression
		{

		} 
	 	THEN statement
	  	{

		}
	  	optional_else
	| 	WHILE expression
		{

		}
	  	DO statement
	  	{

	  	}
	|	FOR ID
		{

		}
		IN ID
		{

		}
		DO statement
		{

		}
	|	FOR variable
		{

		}
		ASSIGN NUM 
		{
			
		}
		inc_or_dec DO statement
		{

		}
	|	REPEAT statement
		{

		}
		UNTIL expression
		{

		}
	;

inc_or_dec:
	TO
	| DOWNTO
	;

read:
	READ '(' identifiers ')'
	{

	}
	;

write:
	WRITE '(' expression_list ')'
	{

	}
	;

call:
	ID
	{

	}
	| ID '(' expression_list ')'
	{

	}
	;

optional_else:
	ELSE statement ';'
	{

	}
	| ';'
	;

expression:
	simple_expression
	{

	}
	| simple_expression RELOP simple_expression
	{

	}
	;

simple_expression:
	term
	| SIGN simple_expression
	| simple_expression SIGN simple_expression
	| simple_expression or_else simple_expression
	| simple_expression OR simple_expression
	;

term:
	factor
	| term and_then term
	| term mulop term
	;

factor:
	variable
	| ID '(' expression_list ')'
	{

	}
	| NUM
	| '(' expression ')'
	| NOT factor
	;

expression_list: 
	comma_expr
	| %empty
	;

variable:
	ID
	{

	}
	| ID dim_exprs
	{

	}
	;

dim_exprs:
	dim_expr
	| dim_comma_expr

dim_expr:
	'[' expression ']'
	| dim_expr '[' expression ']'
	;

dim_comma_expr:
	'[' comma_expr ']'
	;

comma_expr:
	expression
	| comma_expr ',' expression
	;

arguments:
	'(' args_decl ')'
	| %empty
	;

args_decl:
	args_decl ',' arg_decl
	{

	}
	| %empty
	;

arg_decl:
	VAR identifiers ':' type
	{

	}
	| identifiers ':' type
	{

	}
	;

primitives:
	INTEGER
	| REAL
	;

dims:
	range
	{

	}
	| dims ',' range
	{

	}
	;

range:
	NUM '.' '.' NUM
	;

array_decl:
	'[' dims ']' OF primitives
	;

type:
	primitives
	| ARRAY array_decl
	;

and_then:
	AND THEN
	;

or_else:
	OR ELSE
	;

mulop:
	AND
	| MULOP
	;

eof:
	DONE
	{
		return 0;
	}
	;
%%