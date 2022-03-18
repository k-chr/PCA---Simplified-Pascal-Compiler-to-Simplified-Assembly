#include <ostream>

enum class scope 
{
	UNBOUND,
	GLOBAL,
	LOCAL
};

inline std::ostream& operator<<(std::ostream& out, const scope sc)
{
	switch(sc)
	{
        case scope::UNBOUND:
		{
			out << "UNBOUND";
			break;
		}
        case scope::GLOBAL:
		{
			out << "GLOBAL";
			break;
		}
        case scope::LOCAL:
		{
			out << "LOCAL";
        	break;
		}
    }

	return out;
}

enum class local_scope
{
	UNBOUND =-1,
	FUN = 8,
	PROC = 4
};

inline std::ostream& operator<<(std::ostream& out, const local_scope lsc)
{
	switch (lsc) 
	{
        case local_scope::UNBOUND:
		{
			out << "UNBOUND";
			break;
		}
        case local_scope::FUN:
		{
			out << "FUN";
			break;
		}
        case local_scope::PROC:
		{
			out << "PROC";
			break;
		}
    }
	return out;
}

enum class dtype
{
	NONE,
	INT,
	REAL,
	OBJECT
};

inline std::ostream& operator<<(std::ostream& out, const dtype t)
{
	switch (t) 
	{
        case dtype::INT:
		{
			out << "integer";
			break;
		}
        case dtype::REAL:
		{
			out << "real";
			break;
		}
        case dtype::NONE:
		{
			out << "NONE";
			break;
		}
		case dtype::OBJECT:
		{
			out << "object";
			break;
		}
    }

	return out;
}

enum class entry
{
	NONE,
	FUNC,
	PROC,
	VAR,
	NUM,
	ARR,
	RNG,
	OP,
	LABEL,
	TYPE
};

inline std::ostream& operator<<(std::ostream& out, const entry e)
{
	switch (e) 
	{
        case entry::NONE: 
		{
			out << "NONE";
			break;
		}
		case entry::FUNC:
		{
			out << "function";
			break;
		}
		case entry::PROC:
		{
			out << "procedure";
			break;
		}
		case entry::VAR:
		{
			out << "variable";
			break;
		}
		case entry::NUM:
		{
			out << "constant";
			break;
		}
		case entry::ARR:
		{
			out << "array";
			break;
		} 
		case entry::RNG: 
		{
			out << "range";
			break;
		}
		case entry::OP: 
		{
			out << "operator";
			break;
		}
		case entry::LABEL: 
		{
			out << "label";
			break;
		}
		case entry::TYPE: 
		{
			out << "type";
			break;
		}
    }

	return out;
}

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
	NOT,	 //OP:	 not.i arg_1, arg_to	
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
	ENTER,	 //OP:	 enter arg
	INCSP,   //OP:   incsp arg 
	RET,     //OP:   return
	LEAVE,   //OP:   leave
	EXIT     //OP:   exit
};

inline std::ostream& operator<<(std::ostream& out, const opcode opcd)
{
	switch(opcd)
	{
        case opcode::NOP:
		{
			out << "opcode::NOP";
			break;
		}
        case opcode::NOT:
		{
			out << "opcode::NOT";
			break;
		}
        case opcode::ADD:
		{
			out << "opcode::ADD";
			break;
		}
        case opcode::SUB:
		{
			out << "opcode::SUB";
			break;
		}
        case opcode::MUL:
		{
			out << "opcode::MUL";
			break;
		}
        case opcode::DIV:
		{
			out << "opcode::DIV";
			break;
		}
        case opcode::MOD:
		{
			out << "opcode::MOD";
			break;
		}
        case opcode::AND:
		{
			out << "opcode::AND";
			break;
		}
        case opcode::OR:
		{
			out << "opcode::OR";
			break;
		}
        case opcode::NE:
		{
			out << "opcode::NE";
			break;
		}
        case opcode::LE:
		{
			out << "opcode::LE";
			break;
		}
        case opcode::LT:
		{
			out << "opcode::LT";
			break;
		}
        case opcode::GE:
		{
			out << "opcode::GE";
			break;
		}
        case opcode::GT:
		{
			out << "opcode::GT";
			break;
		}
        case opcode::EQ:
		{
			out << "opcode::EQ";
			break;
		}
        case opcode::I2R:
		{
			out << "opcode::I2R";
			break;
		}
        case opcode::R2I:
		{
			out << "opcode::R2I";
			break;
		}
        case opcode::MOV:
		{
			out << "opcode::MOV";
			break;
		}
        case opcode::JMP:
		{
			out << "opcode::JMP";
			break;
		}
        case opcode::WRT:
		{
			out << "opcode::WRT";
			break;
		}
        case opcode::RD:
		{
			out << "opcode::RD";
			break;
		}
        case opcode::PSH:
		{
			out << "opcode::PSH";
			break;
		}
        case opcode::CALL:
		{
			out << "opcode::CALL";
			break;
		}
        case opcode::ENTER:
		{
			out << "opcode::ENTER";
			break;
		}
        case opcode::INCSP:
		{
			out << "opcode::INCSP";
			break;
		}
        case opcode::RET:
		{
			out << "opcode::RET";
			break;
		}
        case opcode::LEAVE:
		{
			out << "opcode::LEAVE";
			break;
		}
        case opcode::EXIT:
		{
			out << "opcode::EXIT";
			break;
		}
    }

    return out;
}

enum class varsize
{
	NONE,
	REF = 4,
	INT = 4,
	REAL = 8
};