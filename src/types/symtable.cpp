#include "symtable.hpp"
#include <algorithm>
#include <memory>
#include <string>


const std::map<std::string, opcode> SymTable::relops_mulops_signops ={
	{"*", 		opcode::MUL},
	{"div", 	opcode::DIV},
	{"/", 		opcode::DIV},
	{"mod",		opcode::MOD},
	{"+", 		opcode::ADD},
	{"-", 		opcode::SUB},
	{"and", 	opcode::AND},
	{"or", 		 opcode::OR},
	{"=", 		 opcode::EQ},
	{"<>", 		 opcode::NE},
	{">=", 		 opcode::GE},
	{">", 		 opcode::GT},
	{">", 		 opcode::GT},
	{"<=", 		 opcode::LE},
	{"<", 		 opcode::LT},
	{":=",		opcode::MOV},
	{"not",		opcode::NOT}
};

const std::map<token, std::string> SymTable::keywords = {
	{token::PROGRAM,  "program"},
	{token::BEGIN_TOK, 	"begin"},
	{token::END,		  "end"},
	{token::VAR, 		  "var"},
	{token::INTEGER,  "integer"},
	{token::REAL,		 "real"},
	{token::ARRAY, 		"array"},
	{token::OF, 		   "of"},
	{token::FUN, 	 "function"},
	{token::PROC, 	"procedure"},
	{token::IF, 		   "if"},
	{token::THEN, 		 "then"},
	{token::ELSE, 		 "else"},
	{token::DO,			   "do"},
	{token::WHILE, 		"while"},
	{token::REPEAT,	   "repeat"},
	{token::UNTIL, 		"until"},
	{token::FOR, 		  "for"},
	{token::IN, 		   "in"},
	{token::TO, 		   "to"},
	{token::DOWNTO,    "downto"},
	{token::WRITE,		"write"},
	{token::READ,		 "read"}
};

std::string SymTable::keyword(const token token)
{
	return SymTable::keywords.at(token);
}

scope SymTable::scope() 
{
	return this->current_scope;
}

local_scope SymTable::local_scope()
{
	return this->scope() == scope::GLOBAL ? local_scope::UNBOUND : this->current_local_scope;
}

void SymTable::leave_global_scope()
{
	if (this->current_scope == scope::GLOBAL)
		this->current_scope = scope::LOCAL;
}

void SymTable::set_local_scope(enum local_scope& value)
{
	if(this->scope() == scope::LOCAL)
	{
		this->current_local_scope = value;
	}
}

void SymTable::return_to_global_scope()
{
	if(this->current_scope == scope::LOCAL)
		this->current_scope = scope::GLOBAL;
}

void SymTable::clear()
{
	this->current_scope = scope::GLOBAL;
	this->checkpoint = 0;
	this->labels.clear();
	this->symbols.clear();
	this->locals = 0;
}

int SymTable::insert(const enum scope& scope, const std::string& name, const entry& entry, const dtype& dtype, int address, bool is_reference, int start, int stop)
{
	int id = this->symbols.size();
	Symbol s = {
					.scope=scope,
					.entry=entry,
					.dtype=dtype,
					.name=name,
					.is_reference = is_reference,
					.symtab_id=id,
					.address=address,
					.start_ind = start,
					.stop_ind = stop
			   };

	this->symbols.push_back(s);
	return id;
}

int SymTable::insert(const std::string& literal, const dtype& dtype)
{
	auto id = this->lookup(literal);
	if(id != SymTable::NONE)
	{
		return id;
	}

	return this->insert(this->scope(), literal, entry::NUM, dtype);
}

int SymTable::insert(const dtype& type, bool is_reference)
{
	auto address = this->get_last_addr();
	if (this->scope() == scope::LOCAL)
	{
		auto offset = 0;
		if (is_reference) 
		{
			offset = static_cast<int>(varsize::REF);
		}
		else 
		{
			switch (type) 
			{
				case dtype::INT:
				{
					offset = static_cast<int>(varsize::INT);
					break;
				}
				case dtype::REAL:
				{
					offset = static_cast<int>(varsize::REAL);
					break;
				}
            	case dtype::NONE:break;
            }
        }
		address -= offset;
	}

	return this->insert(this->scope(), "$t" + std::to_string(this->locals++), entry::VAR, type, address, is_reference);
}

void SymTable::update_var(int id, int type_id)
{
	auto& symbol = this->get(id);

	if(symbol.scope != scope::UNBOUND)
	{
		yyerror(("Syntax error. Redefinition of variable: " + symbol.name + " at line: " + std::to_string(lineno)).c_str());
	}

	auto addr = this->get_last_addr();
	
	symbol.scope = this->scope();
	symbol.address = addr;

	auto none_type = static_cast<int>(dtype::NONE);

	if (type_id >= none_type)
	{
		type_id -= none_type;
		auto arr_type = this->get(type_id);

		if (arr_type.entry != entry::ARR)
		{
			yyerror(("Unknown error, expected ARR entry not: " + std::to_string((int)arr_type.entry) + " at line: " + std::to_string(lineno)).c_str());
		}

		symbol.dtype = arr_type.dtype;
		symbol.args = arr_type.args;
		symbol.entry = arr_type.entry;
		symbol.start_ind = arr_type.start_ind;
		symbol.stop_ind = arr_type.stop_ind;
	}
	else
	{
		symbol.entry = entry::VAR;
		symbol.dtype = dtype(type_id);
	}
}

int SymTable::insert(const std::string& yytext, const token& op, const dtype dtype)
{
	auto id = this->lookup(yytext);

	if(id != SymTable::NONE)
	{
		return id;
	}

	switch (op)
	{
        case token::RELOP:
        case token::MULOP:
        case token::SIGN:
        case token::ASSIGN:
        case token::OR:
		case token::AND:
        case token::NOT:
		{	
			auto it =SymTable::relops_mulops_signops.find(yytext);
			if (it == SymTable::relops_mulops_signops.cend())
			{
				break;
			}

			const auto opcd = SymTable::relops_mulops_signops.at(yytext);
			return this->insert(scope::GLOBAL, yytext, entry::OP, dtype::NONE, static_cast<int>(opcd));
		}

        case token::ID:
			return this->insert(scope::UNBOUND, yytext, entry::NONE, dtype::NONE);

        case token::NUM:
			return this->insert(scope::UNBOUND, yytext, entry::NUM, dtype);

		default: break;
    }
    
	return this->NONE;
}

int SymTable::insert(const std::string& label)
{
	if (this->labels.find(label) == this->labels.cend())
	{
		this->labels[label] = 0;
	}

	auto name = label + std::to_string(this->labels[label]);
	this->labels[label]++;
	return this->insert(this->scope(), name, entry::LABEL, dtype::NONE);
}

int SymTable::insert(int start, int end)
{
	auto start_sym = this->get(start);
	auto end_sym = this->get(end);

	if(start_sym.dtype == dtype::REAL or end_sym.dtype == dtype::REAL)
	{
		yyerror(("Range bound types are not (integer, integer) but got: (" + start_sym.type_to_str() + ", " + end_sym.type_to_str() + "), at line: " + std::to_string(lineno)).c_str());
	}

	auto name = "range(" + start_sym.name + ", " + end_sym.name + ")";
	auto id = this->lookup(name);

	if(id != SymTable::NONE)
	{
		return id;
	}

	start = std::atoi(start_sym.name.c_str());
	end = std::atoi(end_sym.name.c_str());

	auto op = (start <= end ? opcode::ADD : opcode::SUB);

	return this->insert(this->scope(), name, entry::RNG, dtype::INT, static_cast<int>(op), false, start, end);
}

int SymTable::lookup(const std::string& name)
{
	const auto it =std::find_if(this->symbols.cbegin(), this->symbols.cend(), [&name, this](const Symbol& s)
	{
		return s.name == name and (
			s.entry == entry::FUNC or 
			s.entry == entry::RNG or 
			s.entry == entry::PROC or 
			s.entry == entry::OP or 
			s.scope == this->scope()
		);
	});

	return it == this->symbols.cend() ? SymTable::NONE : it - this->symbols.cbegin();
}

Symbol& SymTable::get(const int id)
{
	return this->symbols[id];
}

dtype SymTable::infer_type(Symbol& first, Symbol& second)
{
	if (first.dtype == dtype::NONE or second.dtype == dtype::NONE) return dtype::NONE;

	return first.dtype == second.dtype ? first.dtype : dtype::REAL;
}

void SymTable::create_checkpoint()
{
	this->checkpoint = this->symbols.size();
}

void SymTable::restore_checkpoint()
{	
	int sz = this->symbols.size();
	if (this->checkpoint != sz)
	{
		this->symbols.resize(this->checkpoint);
		this->locals = 0;
	}
}

int SymTable::get_last_addr()
{
	const auto it_to_last_sized = std::find_if(this->symbols.crbegin(), this->symbols.crend(), [](const Symbol& s)
	{
		return s.entry == entry::VAR or s.entry == entry::ARR;
	});

	int addr = 0;

	if (it_to_last_sized != this->symbols.crend())
	{
		if (it_to_last_sized->scope == scope::GLOBAL)
		{
			addr = it_to_last_sized->address + it_to_last_sized->size();
		}
		else if (it_to_last_sized->scope == scope::LOCAL and it_to_last_sized->address < 0)
		{
			addr = it_to_last_sized->address - it_to_last_sized->size();
		}
	}

	return addr;
}