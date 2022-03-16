%{
	#include "global.hpp"
	#include <exception>
	#include <iostream>

	void yyerror(std::exception& exc);
	void yyerror(const char* message);
%}

%output "parser.cpp"

%union	{
	int int_val;
}

%type <int_val> program header program_args variable_decl subprogram_decl block eof identifiers type
subprogram arguments statements statement_list statement args_decl arg_decl primitives array_decl range 
dims variable expression and_then or_else mulop dim_expr dim_exprs dim_comma_expr comma_expr optional_else
optional_args call inc_or_dec expression_list read write simple_expression term factor

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
		emitter_ptr->store_param($1);
	}
	| identifiers ',' ID
	{
		emitter_ptr->store_param($3);
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
		try
		{
			emitter_ptr->assign($1, $3);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
	| block
	| call
	{
		try
		{
			emitter_ptr->make_call($1, false);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
		emitter_ptr->end_parametric_expr();
	}
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
	READ '(' 
	{
		emitter_ptr->begin_parametric_expr();
	} identifiers ')'
	{
		try
		{
			emitter_ptr->read();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
		emitter_ptr->end_parametric_expr();
	}
	;

write:
	WRITE '(' 
	{
		emitter_ptr->begin_parametric_expr();
	} expression_list ')'
	{
		try
		{
			emitter_ptr->write();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
		emitter_ptr->end_parametric_expr();
	}
	;

call:
	ID
	{
		emitter_ptr->begin_parametric_expr();
		$$ = $1;
	}
	| ID '(' 
	{
		$$ = $1;
		emitter_ptr->begin_parametric_expr();
	} expression_list ')'
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
		$$ = $1;
	}
	| simple_expression RELOP simple_expression
	{
		$$ = emitter_ptr->binary_op($2, $1, $3);
	}
	;

simple_expression:
	term
	| SIGN simple_expression
	{
		$$ = emitter_ptr->unary_op($1, $2);
	}
	| simple_expression SIGN simple_expression
	{
		$$ = emitter_ptr->binary_op($2, $1, $3);
	}
	| simple_expression or_else 
	{
		$2 = emitter_ptr->begin_or_else($1);
	} simple_expression
	{
		$$ = emitter_ptr->or_else($2, $3)
	}
	| simple_expression OR simple_expression
	{
		$$ = emitter_ptr->binary_op($2, $1, $3);
	}
	;

term:
	factor
	| term and_then
	{
		$2 = emitter_ptr->begin_and_then($1);
	} term
	{
		$$ = emitter_ptr->and_then($2, $3)
	}
	| term mulop term
	{		
		$$ = emitter_ptr->binary_op($2, $1, $3);
	}
	;

factor:
	variable
	| ID '(' 
	{
		emitter_ptr->begin_parametric_expr();	
	} expression_list ')'
	{
		auto optional_result = emitter_ptr->make_call($1, true);
		$$ = optional_result->get();
		emitter->end_parametric_expr();
	}
	| NUM
	| '(' expression ')'
	| NOT factor
	{
		$$ = emitter_ptr->unary_op($1, $2);
	}
	;

expression_list: 
	comma_expr
	| %empty
	;

variable:
	ID
	{
		emitter_ptr->begin_parametric_expr();
		$$ = emitter_ptr->variable_or_call($1);
		emitter_ptr->end_parametric_expr();
	}
	| ID '[' 
	{
		emitter_ptr->begin_parametric_expr();
	} dim_exprs
	{
		$$ = emitter_ptr->get_item($1);
		emitter_ptr->end_parametric_expr();
	}
	;

dim_exprs:
	expression
	{
		emitter_ptr->store_param($1);
	} ']' dim_expr
	| dim_comma_expr

dim_expr:
	dim_expr '[' expression
	{
		emitter_ptr->store_param($3);
	} ']'
	| %empty
	;

dim_comma_expr:
	comma_expr ']'
	;

comma_expr:
	expression
	{
		emitter_ptr->store_param($1);
	}
	| comma_expr ',' expression
	{
		emitter_ptr->store_param($3);
	}
	;

arguments:
	'(' optional_args ')'
	| %empty
	;

optional_args:
	args_decl
	| %empty
	;

args_decl:
	args_decl ';' arg_decl
	{
		
	}
	| arg_decl
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
	{
		$$ = static_cast<int>(dtype::INT);
	}
	| REAL
	{
		$$ = static_cast<int>(dtype::REAL);
	}
	;

dims:
	range
	{
		emitter_ptr->store_param($1);
	}
	| dims ',' range
	{
		emitter_ptr->store_param($3);
	}
	;

range:
	NUM '.' '.' NUM
	{
		$$ = symtab_ptr->insert($1, $4);
	}
	;

array_decl:
	'[' 
	{
		emitter_ptr->begin_parametric_expr();
	} dims ']' 
	;

type:
	primitives
	| ARRAY array_decl OF primitives
	{
		$$ = symtab_ptr->insert(emitter_ptr->get_params(), dtype($4))
		emitter_ptr->end_parametric_expr();
	}
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

void yyerror(std::exception& exc)
{
	std::cerr << exc.what() << std::endl;
	YYABORT;
}

void yyerror(const char* message)
{
	std::cerr << message << std::endl;
	YYABORT;
}