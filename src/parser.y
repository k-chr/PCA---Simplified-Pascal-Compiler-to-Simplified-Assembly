%{
	#include "global.hpp"
	#include <exception>
	#include <iostream>
	#include <algorithm>
	#include <tuple>

	void yyerror(const std::exception& exc);
	void yyerror(const char* message);

	int opt_else_helper = 0;
%}

%output "parser.cpp"
%verbose
%define parse.error verbose

%union	{
	int int_val;
}

%type <int_val> program header program_args variable_decl subprogram_decl block eof identifiers type
subprogram arguments statements statement_list statement args_decl arg_decl primitives array_decl range 
dims variables variable expression mulop addop dim_exprs comma_expr optional_else optional_prog_args
optional_args call inc_or_dec expression_list read write simple_expression term factor num

%token <int_val> PROGRAM BEGIN_TOK END VAR INTEGER REAL ARRAY OF FUN PROC IF THEN ELSE DO WHILE REPEAT
UNTIL FOR IN TO DOWNTO WRITE READ RANGE RELOP AND_THEN MULOP SIGN ASSIGN AND OR_ELSE OR NOT ID CONST_INT CONST_REAL REAL_FRAG NONE DONE

%nonassoc DANGLING
%nonassoc ELSE

%%

program:
	PROGRAM ID  optional_prog_args  ';' 
	{
		emitter_ptr->begin_parametric_expr();
		emitter_ptr->begin_parametric_expr();
	} variable_decl
	{
		try
		{
			emitter_ptr->end_parametric_expr();
			auto data = emitter_ptr->get_params();
			symtab_ptr->update_addresses(data);
			emitter_ptr->end_parametric_expr();
			emitter_ptr->call_program($2);
			symtab_ptr->leave_global_scope();
			symtab_ptr->create_checkpoint();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
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
			YYABORT;
		}
	}
	block
	'.'
	eof
	;

optional_prog_args:
	'(' program_args ')'
	| %empty
	;

variables:
	variable
	{
		try
		{
			auto res = emitter_ptr->variable_or_call($1, true);
			emitter_ptr->end_parametric_expr();
			emitter_ptr->store_param(res);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	| variables ',' variable
	{
		try
		{
			auto res = emitter_ptr->variable_or_call($3, true);
			emitter_ptr->end_parametric_expr();
			emitter_ptr->store_param(res);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
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
			std::for_each(data.cbegin(), data.cend(), [&computed_type](auto symbol_id)
			{
				symtab_ptr->update_var(symbol_id, computed_type);
				emitter_ptr->store_param_on_stack(symbol_id);
			});
			emitter_ptr->clear_params();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
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
			auto data = emitter_ptr->get_params();
			symtab_ptr->update_addresses(data);
			emitter_ptr->end_parametric_expr(); //basic vector
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	} block
	{
		emitter_ptr->end_current_subprogram($1);
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
			symtab_ptr->update_proc_or_fun($2, entry::FUNC, data, $6);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
		emitter_ptr->end_parametric_expr();
	}
	';' { $$ = $2;}
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
			YYABORT;
		}
		emitter_ptr->end_parametric_expr();
	}
	';' { $$ = $2;}
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
	variable 
	{
		try
		{
			$1 = emitter_ptr->variable_or_call($1, true);
			emitter_ptr->end_parametric_expr();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}

	} ASSIGN expression
	{	
		try
		{
			emitter_ptr->assign($1, $4);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	| block
	| write
	| read
	| call
	{
		try
		{
			emitter_ptr->make_call($1, false);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
		emitter_ptr->end_parametric_expr();
	}
	| 	IF expression
		{
			try
			{
				$2 = emitter_ptr->if_statement($2);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
				YYABORT;
			}
		} 
	 	THEN statement
	  	{
			try
			{
				opt_else_helper = $2;
				$5 = emitter_ptr->end_if();
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
				YYABORT;
			}
		}
	  	optional_else
		{

			if($7 != opt_else_helper)
			{		
				try
				{
					emitter_ptr->label(opt_else_helper);
				}
				catch(const std::exception& exc)
				{
					yyerror(exc);
					YYABORT;
				}
			}

			try
			{
				emitter_ptr->label($5);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
				YYABORT;
			}
		}
	| 	WHILE 
		{
			$1 = emitter_ptr->begin_while();
		} expression
		{
			try
			{
				$3 = emitter_ptr->while_statement($3);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
				YYABORT;
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
				YYABORT;
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
	|	FOR variable 
		{
			try
			{
				$2 = emitter_ptr->variable_or_call($2, true);
				emitter_ptr->end_parametric_expr();
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
				YYABORT;
			}
		} ASSIGN expression inc_or_dec expression
		{
			try
			{
				std::tie($1, $4) = emitter_ptr->classic_for_statement($2, $5, $6, $7);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
				YYABORT;
			}
		} DO statement
		{
			try
			{
				emitter_ptr->classic_end_iteration($2, $6, $1);
				emitter_ptr->label($4);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
				YYABORT;
			}
		}
	|	REPEAT 
		{
			try
			{
				$1 = emitter_ptr->repeat();
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
				YYABORT;
			}
		} statement	UNTIL expression
		{
			try
			{
				emitter_ptr->until($1, $5);
			}
			catch(const std::exception& exc)
			{
				yyerror(exc);
				YYABORT;
			}
		}
	;

inc_or_dec:
	TO
	{
		$$ = static_cast<int>(opcode::ADD);
	}
	| DOWNTO
	{
		$$ = static_cast<int>(opcode::SUB);
	}
	;

optional_else:
	ELSE 
	{
		try
		{
			emitter_ptr->label(opt_else_helper);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	} statement 
	{
		$$ = opt_else_helper;
	}
	| %empty  %prec DANGLING
	;

read:
	READ '(' 
	{
		emitter_ptr->begin_parametric_expr();
	} variables ')'
	{
		try
		{
			emitter_ptr->read();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
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
			YYABORT;
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
		emitter_ptr->begin_parametric_expr();
	} expression_list ')'
	{
		$$ = $1;
	}
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
			YYABORT;
		}
	}
	| simple_expression OR_ELSE
	{
		try
		{
			$2 = emitter_ptr->begin_or_else($1);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	} simple_expression
	{
		try
		{
			$$ = emitter_ptr->or_else($2, $4);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	;

simple_expression:
	term
	{
		$$ = $1;
	}
	| SIGN term
	{
		try
		{
			$$ = emitter_ptr->unary_op($1, $2);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	| simple_expression addop term
	{
		try
		{
			$$ = emitter_ptr->binary_op($2, $1, $3);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	;

term:
	factor
	{
		$$ = $1;
	}
	| term AND_THEN
	{
		try
		{
			$2 = emitter_ptr->begin_and_then($1);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	} factor
	{
		try
		{
			$$ = emitter_ptr->and_then($2, $4);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	| term mulop factor
	{	
		try
		{
			$$ = emitter_ptr->binary_op($2, $1, $3);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}	
	}
	;

factor:
	variable 
	{
		try
		{
			$$ = emitter_ptr->variable_or_call($1);
			emitter_ptr->end_parametric_expr();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	| ID '(' 
	{
		emitter_ptr->begin_parametric_expr();	
	} expression_list ')'
	{
		try
		{
			auto optional_result = emitter_ptr->make_call($1, true);
			$$ = optional_result.value_or(SymTable::NONE);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
		emitter_ptr->end_parametric_expr();
	}
	| num
	| '(' expression ')'
	{
		$$ = $2;
	}
	| NOT factor
	{
		try
		{
			$$ = emitter_ptr->unary_op($1, $2);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
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
			$$ = symtab_ptr->check_symbol($1, true).symtab_id;
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	| ID dim_exprs
	{
		try
		{
			$$ = symtab_ptr->check_symbol($1, true).symtab_id;
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	;

dim_exprs:
	'[' 
	{
		emitter_ptr->begin_parametric_expr();
	} comma_expr ']'
	

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
			auto computed_type = $4;
			std::for_each(data.cbegin(), data.cend(), [&computed_type](auto symbol_id){
				symtab_ptr->update_var(symbol_id, computed_type, true);
				emitter_ptr->store_param_on_stack(symbol_id);
			});
			emitter_ptr->clear_params();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
	}
	| identifiers ':' type
	{
		try
		{
			auto data = emitter_ptr->get_params();
			auto computed_type = $3;
			std::for_each(data.cbegin(), data.cend(), [&computed_type](auto symbol_id){
				symtab_ptr->update_var(symbol_id, computed_type);
				emitter_ptr->store_param_on_stack(symbol_id);
			});
			emitter_ptr->clear_params();
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
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
	CONST_INT RANGE CONST_INT
	{
		try
		{
			$$ = symtab_ptr->insert_range($1, $3);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}
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
			auto data = emitter_ptr->get_params();
			auto type = dtype($4);
			$$ = symtab_ptr->insert_array_type(data, type);
		}
		catch(const std::exception& exc)
		{
			yyerror(exc);
			YYABORT;
		}

		emitter_ptr->end_parametric_expr();
	}
	;

num:
	CONST_INT
	| CONST_REAL
	| REAL_FRAG CONST_REAL 
	{
		$$ = $2;
	}
	;

addop:
	OR
	| SIGN
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

void yyerror(const std::exception& exc)
{
	std::cerr << exc.what() << std::endl;
}

void yyerror(const char* message)
{
	std::cerr << message << ".\tAt line: " << lineno << std::endl;
}