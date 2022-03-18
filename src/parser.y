%{
	#include "global.hpp"
	#include <exception>
	#include <iostream>
	#include <algorithm>
	#include <tuple>

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
	PROGRAM ID '(' program_args ')' ';' 
	{
		emitter_ptr->begin_parametric_expr();
		emitter_ptr->begin_parametric_expr();
	} variable_decl
	{
		try
		{
			emitter_ptr->end_parametric_expr();
			symtab_ptr->update_addresses(emitter_ptr->get_params());
			emitter_ptr->end_parametric_expr();
			emitter_ptr->call_program($2);
			symtab_ptr->leave_global_scope();
			symtab_ptr->create_checkpoint();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}

	} subprogram_decl
	{
		symtab_ptr->return_to_global_scope();
		try
		{
			emitter_ptr->start_program($2);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
	block
	'.'
	eof
	;

program_args:
	ID
	| program_args ',' ID
	;

variable_decl:
	variable_decl VAR identifiers ':' type ';'
	{
		try
		{
			auto data = emitter_ptr->get_params();
			auto computed_type = $5;
			std::for_each(data.crbegin(), data.crend(), [symtab_ptr, emitter_ptr, computed_type](auto symbol_id)
			{
				symtab_ptr->update_var(symbol_id, computed_type);
				emitter_ptr->store_param_on_stack(symbol_id);
			});
			emitter_ptr->clear_params();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
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
	header 
	{
		emitter_ptr->begin_parametric_expr();
		emitter_ptr->begin_parametric_expr();
	} variable_decl
	{
		try
		{
			emitter_ptr->end_parametric_expr();//stack of variables
			symtab_ptr->update_addresses(emitter_ptr->get_params());
			emitter_ptr->end_parametric_expr(); //basic vector
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	} block
	{
		emitter_ptr->end_current_subprogram();
	}
	;

header:
	FUN ID
	{
		symtab_ptr->leave_global_scope();
		symtab_ptr->create_checkpoint();
		symtab_ptr->set_local_scope(local_scope::FUN);
	}
	arguments
	':' type
	{
		auto data = emitter_ptr->get_params();
		try
		{
			symtab_ptr->update_proc_or_fun($2, entry::PROC, data, $5);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
		emitter_ptr->end_parametric_expr();
	}
	';'
	| PROC ID
	{
		symtab_ptr->leave_global_scope();
		symtab_ptr->create_checkpoint();
		symtab_ptr->set_local_scope(local_scope::PROC);
	}
	arguments
	{	
		auto data = emitter_ptr->get_params();
		try
		{
			symtab_ptr->update_proc_or_fun($2, entry::PROC, data);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
		emitter_ptr->end_parametric_expr();
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
			try
			{
				$5 = emitter_ptr->if_statement($2);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
			}
		} 
	 	THEN statement
	  	{
			try
			{
				$4 = emitter_ptr->end_if();
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
			}
		}
	  	optional_else
		{
			try
			{
				emitter_ptr->label($4);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
			}
		}
	| 	WHILE expression
		{
			try
			{
				std::tie($1, $3) = emitter_ptr->while_statement($2);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
			}
		}
	  	DO statement
	  	{
			try
			{
				emitter_ptr->jump($1);
				emitter_ptr->label($3);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
			}
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
	|	FOR variable ASSIGN expression inc_or_dec expression
		{
			try
			{
				std::tie($1, $3) = emitter_ptr->classic_for_statement($2, $4, $5, $6);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
			}
		} DO statement
		{
			try
			{
				emitter_ptr->classic_end_interation($2, $5, $1);
				emitter_ptr->label($3);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
			}
		}
	|	REPEAT statement
		{
			try
			{
				$1 = emitter_ptr->repeat();
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
			}
		}
		UNTIL expression
		{
			try
			{
				emitter_ptr->until($1, $2);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
			}
		}
	;

inc_or_dec:
	TO
	{
		$$ = static_cast<int>opcode::ADD;
	}
	| DOWNTO
	{
		$$ = static_cast<int>opcode::SUB;
	}
	;

optional_else:
	ELSE 
	{
		try
		{
			emitter_ptr->label($$);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	} statement ';'
	| ';'
	{
		try
		{
			emitter_ptr->label($$);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
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

expression:
	simple_expression
	{
		$$ = $1;
	}
	| simple_expression RELOP simple_expression
	{
		try
		{
			$$ = emitter_ptr->binary_op($2, $1, $3);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
	;

simple_expression:
	term
	| SIGN simple_expression
	{
		try
		{
			$$ = emitter_ptr->unary_op($1, $2);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
	| simple_expression SIGN simple_expression
	{
		try
		{
			$$ = emitter_ptr->binary_op($2, $1, $3);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
	| simple_expression or_else 
	{
		try
		{
			$2 = emitter_ptr->begin_or_else($1);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	} simple_expression
	{
		try
		{
			$$ = emitter_ptr->or_else($2, $3)
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
	| simple_expression OR simple_expression
	{
		try
		{
			$$ = emitter_ptr->binary_op($2, $1, $3);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
	;

term:
	factor
	| term and_then
	{
		try
		{
			$2 = emitter_ptr->begin_and_then($1);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	} term
	{
		try
		{
			$$ = emitter_ptr->and_then($2, $3)
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
	| term mulop term
	{	
		try
		{
			$$ = emitter_ptr->binary_op($2, $1, $3);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}	
	}
	;

factor:
	variable
	| ID '(' 
	{
		emitter_ptr->begin_parametric_expr();	
	} expression_list ')'
	{
		try
		{
			auto optional_result = emitter_ptr->make_call($1, true);
			$$ = optional_result->value_or(SymTable::NONE);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
		emitter->end_parametric_expr();
	}
	| NUM
	| '(' expression ')'
	| NOT factor
	{
		try
		{
			$$ = emitter_ptr->unary_op($1, $2);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
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

		try
		{
			$$ = emitter_ptr->variable_or_call($1);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}

		emitter_ptr->end_parametric_expr();
	}
	| ID '[' 
	{
		emitter_ptr->begin_parametric_expr();
	} dim_exprs
	{
		try
		{
			$$ = emitter_ptr->get_item($1);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}

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
	'(' 
	{
		emitter_ptr->begin_parametric_expr();
		emitter_ptr->begin_parametric_expr();
	} optional_args ')'
	{
		emitter_ptr->end_parametric_expr();
	}
	| %empty
	;

optional_args:
	args_decl
	| %empty
	;

args_decl:
	args_decl ';' arg_decl
	| arg_decl
	;

arg_decl:
	VAR identifiers ':' type
	{
		try
		{
			auto data = emitter_ptr->get_params();
			auto computed_type = $3;
			std::for_each(data.crbegin(), data.crend(), [symtab_ptr, emitter_ptr, computed_type](auto symbol_id){
				symtab_ptr->update_var(symbol_id, computed_type, true);
				emitter_ptr->store_param_on_stack(symbol_id);
			});
			emitter_ptr->clear_params();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
	}
	| identifiers ':' type
	{
		try
		{
			auto data = emitter_ptr->get_params();
			auto computed_type = $3;
			std::for_each(data.crbegin(), data.crend(), [symtab_ptr, emitter_ptr, computed_type](auto symbol_id){
				symtab_ptr->update_var(symbol_id, computed_type);
				emitter_ptr->store_param_on_stack(symbol_id);
			});
			emitter_ptr->clear_params();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}
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
		try
		{
			$$ = symtab_ptr->insert_range($1, $4);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}S
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
		try
		{
			$$ = symtab_ptr->insert_array_type(emitter_ptr->get_params(), dtype($4))
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
		}

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
		emitter_ptr->end_program();
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