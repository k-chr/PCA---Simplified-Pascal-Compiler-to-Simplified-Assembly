%option outfile="lexer.cpp"

%option noyywrap
%{
	#include "global.hpp"
	#include "parser.hpp"
	#include <string>
	#include <cstdio>
	extern YYSTYPE yylval;
	extern std::string backup;
%}

%s fnum
/* keywords and operators definitions */
dots		".."
number      {digits}
digit		[0-9]
digits		{digit}+
program		"program"
var			"var"
array		"array"
of			"of"
integer		"integer"
real		"real"
read		"read"
write		"write"
function	"function"
procedure	"procedure"
begin		"begin"
end			"end"
if			"if"
then		"then"
else		"else"
while		"while"
do			"do"
repeat		"repeat"
until		"until"
for			"for"
to			"to"
downto		"downto"
in			"in"
not			"not"
mulop		"*"|"/"|"div"|"mod"
and_then	"and"{space}"then"
and 		"and"
sign		"-"|"+"
or_else		"or"{space}"else"
or			"or"
relop		"<>"|"<="|">="|"<"|">"|"="
assign		":="
/* regex definitions */
space		[ \t]
newline		[\r]?[\n]
blank		{space}+
exponential [eE][+-]?{digits}
fract    	({digits}{exponential}?)|({exponential})
floating	{digits}{exponential}
alpha		[a-zA-Z]
id			({alpha}|[_])({alpha}|{digit}|[_])*
other		.

%%
{blank}						;
{newline}					{
								++lineno;
							}
{program}					{
								return token::PROGRAM;
							}	
{var}						{
								return token::VAR;
							}
{array}						{
								return token::ARRAY;
							}
{of}						{
								return token::OF;
							}
{integer}					{
								return token::INTEGER;
							}
{real}						{
								return token::REAL;
							}
{function}					{
								return token::FUN;
							}
{procedure}					{
								return token::PROC;
							}
{begin}						{
								return token::BEGIN_TOK;
							}
{end}						{
								return token::END;
							}
{read}						{
								return token::READ;
							}
{write}						{ 
								return token::WRITE;
							}
{if}						{
								return token::IF;
							}
{then}						{
								return token::THEN;
							}
{else}						{
								return token::ELSE;
							}
{while}						{
								return token::WHILE;
							}
{do}						{
								return token::DO;
							}
{repeat}					{
								return token::REPEAT;
							}
{until}						{
								return token::UNTIL;
							}
{for}						{
								return token::FOR;
							}
{to}						{
								return token::TO;
							}
{downto}					{
								return token::DOWNTO;
							}
{in}						{
								return token::IN;
							}
{not}						{
								yylval.int_val = static_cast<int>(symtab_ptr->op(yytext));
								return token::NOT;
							}
{and_then}					{
								yylval.int_val = static_cast<int>(symtab_ptr->op(yytext));
								return token::AND_THEN;
							}
{mulop}						{
								yylval.int_val = static_cast<int>(symtab_ptr->op(yytext));
								return token::MULOP;
							}
{sign}						{
								yylval.int_val = static_cast<int>(symtab_ptr->op(yytext));
								return token::SIGN;
							}
{or_else}					{
								yylval.int_val = static_cast<int>(symtab_ptr->op(yytext));
								return token::OR_ELSE;
							}
{or}						{
								yylval.int_val = static_cast<int>(symtab_ptr->op(yytext));
								return token::OR;
							}
{and}						{
								yylval.int_val = static_cast<int>(symtab_ptr->op(yytext));
								return token::AND;
							}
{relop}						{
								yylval.int_val = static_cast<int>(symtab_ptr->op(yytext));
								return token::RELOP;
							}
{assign}					{
								yylval.int_val = static_cast<int>(symtab_ptr->op(yytext));
								return token::ASSIGN;
							}
{id}						{
								yylval.int_val = symtab_ptr->insert_by_token(yytext, token::ID);
								return token::ID;
							}
{number}					{	
								yylval.int_val = symtab_ptr->insert_by_token(yytext, token::CONST_INT, dtype::INT);
								return token::CONST_INT;
							}
{dots}						{
								return token::RANGE;
							}
{number}"."/([;]|{space})	{
								yylval.int_val = symtab_ptr->insert_by_token(yytext, token::CONST_REAL, dtype::REAL);
								return token::CONST_REAL;
							}								
{number}"."/[^.]			{
								backup = yytext;
								std::cout<< "begin float" << std::endl;
								BEGIN fnum;
								return token::REAL_FRAG;
							}
{floating}					{
								yylval.int_val = symtab_ptr->insert_by_token(yytext, token::CONST_REAL, dtype::REAL);
								return token::CONST_REAL;
							}
<fnum>{fract}/([;]|{space})	{
								auto combined = (backup + yytext);
								if (combined.empty())
								{
									return ';';
								}
								std::cout << std::endl << combined << std::endl;
								yylval.int_val = symtab_ptr->insert_by_token(combined.c_str(), token::CONST_REAL, dtype::REAL);
								backup.clear();
								return token::CONST_REAL;
							}						
<<EOF>>						{
								return token::DONE;
							}
{other}						{
								return *yytext;
							}
%%