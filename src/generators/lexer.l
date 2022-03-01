%option outfile="lexer.cpp"

%option noyywrap
%{
    #include "global.hpp"

%}
/* keywords definitions */
program 	"program"
var 		"var"
array 		"array"
of 			"of"
integer		"integer"
real		"real"
function	"function"
procedure	"procedure"
begin		"begin"
end			"end"
if 			"if"
then		"then"
else		"else"
while		"while"
do 			"do"
repeat		"repeat"
until		"until"
for			"for"
to 			"to"
downto		"downto"
in  		"in"
not			"not"
mulop		"*"|"/"|"div"|"mod"|"and"
sign		"-"|"+"
or 			"or"
relop		"<>"|"<="|">="|"<"|">"|"="
assign		":="
/* regex definitions */
space	    [ \t]
newline     [\n]
blank	    {space}+
digit	    [0-9]
digits		{digit}+
floating	{digits}(\.{digits}?)([eE][+-]?{digits})?
alpha	    [a-zA-Z]
id		    {alpha}({alpha}|{digit})*
other		.

%%
{blank}		;
{newline}	{
				++lineno;
			}
{program}	{
				return {.token_val=token::PROGRAM};
			}	
{var}		{
				return {.token_val=token::VAR};
			}
{array}		{
				return {.token_val=token::ARRAY};
			}
{of}		{
				return {.token_val=token::OF};
			}
{integer}	{
				return {.token_val=token::INTEGER};
			}
{real}		{
				return {.token_val=token::REAL};
			}
{function}	{
				return {.token_val=token::FUN};
			}
{procedure}	{
				return {.token_val=token::PROC};
			}
{begin}		{
				return {.token_val=token::BEGIN};
			}
{end}		{
				return {.token_val=token::END};
			}
{if} 		{
				return {.token_val=token::IF};
			}	
{then}		{
				return {.token_val=token::THEN};
			}
{else}		{
				return {.token_val=token::ELSE};
			}
{while}		{
				return {.token_val=token::WHILE};
			}
{do} 		{
				return {.token_val=token::DO};
			}	
{repeat}	{
				return {.token_val=token::REPEAT};
			}
{until}		{
				return {.token_val=token::UNTIL};
			}
{for}		{
				return {.token_val=token::FOR};
			}
{to} 		{
				return {.token_val=token::TO};
			}
{downto}	{
				return {.token_val=token::DOWNTO};
			}
{in}  		{
				return {.token_val=token::IN};
			}
{not}		{
				return {.token_val=token::NOT};
			}
{mulop}		{
				return {.token_val=token::MULOP, .int_val=};
			}
{sign}		
{or} 			
{relop}
{assign}
{id}		{
				if(yyleng >= BSIZE) error("Exceeded maximum identifier length");
				int idx = lookup(yytext);
				if (idx == 0)
					idx = insert(yytext, ID);
				tokenval = idx;
				return symtable[idx].token;
			}
{digits}	{
				tokenval = atoi(yytext);
				return NUM;
			}
{floating}	{

			}
<<EOF>>	    {
				return DONE;
			}	
{other}		{
				return *yytext;
			}
%%
int lexan()
{
	return yylex();
}