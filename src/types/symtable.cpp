#include "symtable.hpp"
#include <algorithm>
#include <string>

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

int SymTable::insert(const std::string& literal, const dtype& dtype)
{
	auto id = this->lookup(literal);
	if(id != SymTable::NONE)
	{
		return id;
	}

	id = this->symbols.size();

	Symbol s = {	
					.scope=this->scope(),
					.entry=entry::NUM,
					.dtype=dtype,
					.token=token::NONE,
					.name=literal,
					.symtab_id=id
				};

	this->symbols.push_back(s);
	return id;
}

int SymTable::insert(const dtype& type)
{
	int id = this->symbols.size();
	Symbol s = {
					.scope=this->scope(),
					.entry=entry::VAR,
					.dtype=type,
					.token=token::NONE,
					.name= "$t" + std::to_string(this->locals++),
					.symtab_id=id
			   };

	this->symbols.push_back(s);
	return id;
}

int SymTable::insert(const std::string& name, const entry& entry, const token& token, const dtype& dtype)
{
	int id = this->symbols.size();
	Symbol s = {
					.scope=this->scope(),
					.entry=entry,
					.dtype=dtype,
					.token=token,
					.name=name,
					.symtab_id=id
			   };

	this->symbols.push_back(s);
	return id;
}

int SymTable::insert(const std::string& label)
{
	int id = this->symbols.size();
	if (this->labels.find(label) == this->labels.cend()){
		this->labels[label] = 0;
	}
	auto name = label + std::to_string(this->labels[label]);
	this->labels[label]++;

	Symbol s = {
					.scope=this->scope(),
					.entry=entry::LABEL,
					.dtype=dtype::NONE,
					.token=token::NONE,
					.name=name,
					.symtab_id = id
			   };

	this->symbols.push_back(s);
	return id;
}

int SymTable::lookup(const std::string& name)
{
	const auto it =std::find_if(this->symbols.cbegin(), this->symbols.cend(), [&name, this](const Symbol& s){
		return s.name == name and (
			s.entry == entry::FUNC or s.entry == entry::PROC or s.scope == this->scope()
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
	if (this->checkpoint != sz) this->symbols.resize(this->checkpoint);
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