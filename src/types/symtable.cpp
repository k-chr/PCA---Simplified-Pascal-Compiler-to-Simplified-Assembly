#include "symtable.hpp"
#include <algorithm>
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
	{":=",		opcode::MOV}
};

scope SymTable::scope() 
{
	return this->current_scope;
}

void SymTable::leave_global_scope()
{
	if (this->current_scope == scope::GLOBAL)
		this->current_scope = scope::LOCAL;
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

int SymTable::insert(const enum scope& scope, const std::string& name, const entry& entry, const dtype& dtype, int address)
{
	int id = this->symbols.size();
	Symbol s = {
					.scope=scope,
					.entry=entry,
					.dtype=dtype,
					.name=name,
					.symtab_id=id,
					.address=address
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

int SymTable::insert(const dtype& type)
{
	return this->insert(this->scope(), "$t" + std::to_string(this->locals++), entry::VAR, type);
}

int SymTable::insert(const std::string& yytext, const token& op, const dtype dtype)
{
	switch (op)
	{
        case token::RELOP:
        case token::MULOP:
        case token::SIGN:
        case token::ASSIGN:
        case token::OR:
        case token::NOT:
		{
			auto id = this->lookup(yytext);

			if(id != SymTable::NONE)
			{
				return id;
			}

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
	if (this->labels.find(label) == this->labels.cend()){
		this->labels[label] = 0;
	}
	auto name = label + std::to_string(this->labels[label]);
	this->labels[label]++;
	return this->insert(this->scope(), name, entry::LABEL, dtype::NONE);
}

int SymTable::lookup(const std::string& name)
{
	const auto it =std::find_if(this->symbols.cbegin(), this->symbols.cend(), [&name, this](const Symbol& s){
		return s.name == name and (
			s.entry == entry::FUNC or 
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
	const auto it_to_last_sized = std::find_if(this->symbols.crbegin(), this->symbols.crend(), [](const Symbol& s){
		return s.entry == entry::VAR or s.entry == entry::ARR;
	});

	int addr = 0;

	if (it_to_last_sized != this->symbols.crend())
	{
		if (it_to_last_sized->scope == scope::GLOBAL)
		{
			addr = it_to_last_sized->address + it_to_last_sized->size();
		}
		else if (it_to_last_sized->scope == scope::LOCAL)
		{
			addr = it_to_last_sized->address - it_to_last_sized->size();
		}
	}

	return addr;
}