%option outfile="lexer.cpp"

%option noyywrap
%{
	#include "global.hpp"
	#include "parser.hpp"
	#include <string>
	extern YYSTYPE yylval;
%}
/* keywords and operators definitions */
program		"program"
var			"var"
array		"array"
of			"of"
integer		"integer"
real		"real"
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
newline		[\n]
blank		{space}+
digit		[0-9]
digits		{digit}+
floating	{digits}(\.{digits}?)([eE][+-]?{digits})?
alpha		[a-zA-Z]
id			({alpha}|[_])({alpha}|{digit}|[_])*
other		.

%%
{blank}		;
{newline}	{
				++lineno;
			}
{program}	{
				return token::PROGRAM;
			}	
{var}		{
				return token::VAR;
			}
{array}		{
				return token::ARRAY;
			}
{of}		{
				return token::OF;
			}
{integer}	{
				return token::INTEGER;
			}
{real}		{
				return token::REAL;
			}
{function}	{
				return token::FUN;
			}
{procedure}	{
				return token::PROC;
			}
{begin}		{
				return token::BEGIN_TOK;
			}
{end}		{
				return token::END;
			}
{if}		{
				return token::IF;
			}
{then}		{
				return token::THEN;
			}
{else}		{
				return token::ELSE;
			}
{while}		{
				return token::WHILE;
			}
{do}		{
				return token::DO;
			}
{repeat}	{
				return token::REPEAT;
			}
{until}		{
				return token::UNTIL;
			}
{for}		{
				return token::FOR;
			}
{to}		{
				return token::TO;
			}
{downto}	{
				return token::DOWNTO;
			}
{in}		{
				return token::IN;
			}
{not}		{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::NOT);
				return token::NOT;
			}
{and_then}	{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::AND_THEN);
				return token::AND_THEN;
			}
{mulop}		{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::MULOP);
				return token::MULOP;
			}
{sign}		{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::SIGN);
				return token::SIGN;
			}
{or_else}	{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::OR_ELSE);
				return token::OR_ELSE;
			}
{or}		{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::OR);
				return token::OR;
			}
{and}		{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::AND);
				return token::AND;
			}
{relop}		{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::RELOP);
				return token::RELOP;
			}
{assign}	{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::ASSIGN);
				return token::ASSIGN;
			}
{id}		{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::ID);
				return token::ID;
			}
{digits}	{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::NUM, dtype::INT);
				return token::NUM;
			}
{floating}	{
				yylval.int_val = symtab_ptr->insert_by_token(yytext, token::NUM, dtype::REAL);
				return token::NUM;
			}
<<EOF>>		{
				return token::DONE;
			}
{other}		{
				return *yytext;
			}
%%