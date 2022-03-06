enum class scope 
{
	UNBOUND,
	GLOBAL,
	LOCAL
};

enum class dtype
{
	NONE,
	INT,
	REAL
};

enum class entry
{
	NONE,
	FUNC,
	PROC,
	VAR,
	NUM,
	ARR,
	OP,
	LABEL
};

#ifndef YYTOKENTYPE
#define YYTOKENTYPE
enum token
{                
	PROGRAM=258,
    BEGIN_TOK,
    END,
    VAR,
    INTEGER,
    REAL,
    ARRAY,
    OF,
    FUN,
    PROC,
    IF,
    THEN,
    ELSE,
    DO,
    WHILE,
	REPEAT,
	UNTIL,
	FOR,
	IN,
	TO,
	DOWNTO,
	WRITE,
	READ,
    RELOP,
    MULOP,
    SIGN,
    ASSIGN,
	AND,
    OR,
    NOT,
    ID,
    NUM,
    NONE,
    DONE
};
#endif



enum class opcode
{
	NOP,
	ADD,     //OP:   add.t arg_1, arg_2, arg_to
	SUB,     //OP:   sub.t arg_1, arg_2, arg_to
	MUL,     //OP:   mul.t arg_1, arg_2, arg_to
	DIV,     //OP:   div.t arg_1, arg_2, arg_to
	MOD,     //OP:   mod.i arg_1, arg_2, arg_to
	AND,     //OP:   and.i arg_1, arg_2, arg_to
	OR,      //OP:   or.i arg_1, arg_2, arg_to
	NE,      //OP:   jne.i arg_1, arg_2, arg_to
	LE,      //OP:   jle.i arg_1, arg_2, arg_to
	LT,      //OP:   jl.i arg_1, arg_2, arg_to
	GE,      //OP:   jge.i arg_1, arg_2, arg_to
	GT,      //OP:   jg.i arg_1, arg_2, arg_to
	EQ,      //OP: 	 je.i arg_1, arg_2, arg_to
	I2R,     //OP:   inttoreal.r arg_to_cast, arg_to_store
	R2I,     //OP:   realtoint.i arg_to_cast, arg_to_store
	MOV,     //OP:   mov.t arg_from, arg_to
	JMP,     //OP:   jump arg
	WRT,     //OP:   write.t arg
	RD,      //OP:   read.t arg
	PSH,     //OP:   push.t arg
	CALL,    //OP:   call arg
	INCSP,   //OP:   incsp arg 
	RET,     //OP:   return
	LEAVE,   //OP:   leave
	EXIT     //OP:   exit
};

enum class varsize
{
	NONE,
	REF = 4,
	INT = 4,
	REAL = 8
};