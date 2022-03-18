#include "symtable.hpp"

#include <iomanip>
#include <ios>

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

scope SymTable::get_scope() 
{
	return this->current_scope;
}

local_scope SymTable::get_local_scope()
{
	return this->get_scope() == scope::GLOBAL ? local_scope::UNBOUND : this->current_local_scope;
}

void SymTable::leave_global_scope()
{
	if (this->current_scope == scope::GLOBAL)
		this->current_scope = scope::LOCAL;
}

void SymTable::set_local_scope(enum local_scope value)
{
	if(this->get_scope() == scope::LOCAL)
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
					.m_scope=scope,
					.m_entry=entry,
					.m_dtype=dtype,
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

int SymTable::insert_constant(const std::string& literal, const dtype& dtype)
{
	auto id = this->lookup(literal);
	if(id != SymTable::NONE)
	{
		return id;
	}

	return this->insert(this->get_scope(), literal, entry::NUM, dtype);
}

int SymTable::insert_temp(const dtype& type, bool is_reference)
{
	auto address = this->get_last_addr();
	if (this->get_scope() == scope::LOCAL)
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
            	default:break;
            }
        }
		address -= offset;
	}

	return this->insert(this->get_scope(), "$t" + std::to_string(this->locals++), entry::VAR, type, address, is_reference);
}

int SymTable::insert_by_token(const std::string& yytext, const token& op, const dtype dtype)
{
	auto id = this->lookup(yytext);

	if(id != SymTable::NONE)
	{
		return id;
	}

	switch (op)
	{
        case token::RELOP:
		case token::AND_THEN:
		case token::OR_ELSE:
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
			return this->insert_constant(yytext, dtype);

		default: break;
    }
    
	return this->NONE;
}

int SymTable::insert_label(const std::string& label)
{
	if (this->labels.find(label) == this->labels.cend())
	{
		this->labels[label] = 0;
	}

	auto name = label + std::to_string(this->labels[label]);
	this->labels[label]++;
	return this->insert(this->get_scope(), name, entry::LABEL, dtype::NONE);
}

int SymTable::insert_range(int start, int end)
{
	auto start_sym = this->get(start);
	auto end_sym = this->get(end);

	if(start_sym.m_dtype == dtype::REAL or end_sym.m_dtype == dtype::REAL)
	{
		throw CompilerException(interpolate("Syntax error. Range bound types are not (integer, integer) but got: ({0}, {1})", start_sym.type_to_str(), end_sym.type_to_str()), lineno);
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

	return this->insert(this->get_scope(), name, entry::RNG, dtype::INT, static_cast<int>(op), false, start, end);
}

int SymTable::insert_array_type(std::vector<Symbol>& symbols_vec, dtype& type)
{
	std::stringstream ss("array [");
	int sum = 0;
	std::for_each(symbols.cbegin(), symbols.cend(), [&ss, &sum](const auto& symbol)
	{
		ss << symbol.start_ind;
		ss << "..";
		ss << symbol.stop_ind;
		ss << ", ";

		sum += std::abs(symbol.stop_ind-symbol.start_ind);
	});

	ss.seekp(-2, std::ios_base::cur);

	ss << "]";

	ss << " of " << (type == dtype::INT ? "integer" : "real");
	
	auto name = ss.str();
	
	if (int out_id = this->lookup(name); out_id != SymTable::NONE)
	{
		return out_id + static_cast<int>(dtype::OBJECT);
	}

	auto id = this->insert(this->get_scope(), name, entry::TYPE, type, static_cast<int>(entry::ARR), false);
	auto& symbol = this->get(id);

	symbol.args = symbols_vec;
	symbol.start_ind = 0;
	symbol.stop_ind = sum; 

	return symbol.symtab_id + static_cast<int>(dtype::OBJECT);
}

int SymTable::insert_array_type(std::vector<int> dims, dtype& type)
{
	std::vector<Symbol> symbols_vec;

	std::transform(dims.crbegin(), dims.crend(), std::inserter(symbols_vec, symbols_vec.begin()), [this](int id)
	{
		return this->get(id);
	});

	if(std::any_of(symbols_vec.cbegin(), symbols_vec.cend(), [](const auto& symbol)
	{
		return symbol.m_entry != entry::RNG or static_cast<opcode>(symbol.address) != opcode::ADD;
	}))
	{
		throw CompilerException("Syntax error. Expected ascending range in array definition.", lineno);
	}

	return this->insert_array_type(symbols_vec, type);	
}

Symbol& SymTable::check_symbol(int id)
{
	auto& symbol = this->get(id);

	if(symbol.m_scope != scope::UNBOUND)
	{
		throw CompilerException(interpolate("Syntax error. Redefinition of {0}: {1}", symbol.m_entry, symbol.name), lineno);
	}

	return symbol;
}

void SymTable::update_var(int id, int type_id, bool is_reference)
{
	auto& symbol = this->check_symbol(id);
	
	symbol.m_scope = this->get_scope();
	symbol.is_reference = is_reference;
	auto offset = static_cast<int>(dtype::OBJECT);

	if (type_id >= offset)
	{
		type_id -= offset;
		auto arr_type = this->get(type_id);

		if (arr_type.m_entry != entry::TYPE)
		{
			throw CompilerException(interpolate("Unknown error, expected TYPE entry not: {0}", arr_type.m_entry), lineno);
		}

		symbol.m_dtype = arr_type.m_dtype;
		symbol.args = {arr_type};
		symbol.m_entry = static_cast<entry>(arr_type.address);
		symbol.start_ind = arr_type.start_ind;
		symbol.stop_ind = arr_type.stop_ind;
	}
	else
	{
		symbol.m_entry = entry::VAR;
		symbol.m_dtype = dtype(type_id);
	}
}

void SymTable::update_addresses(std::vector<int>& args)
{
	std::vector<Symbol> arg_symbols;
	std::transform(args.crbegin(), args.crend(), std::inserter(arg_symbols, arg_symbols.begin()), [this](auto sym_id){return this->get(sym_id);});

	std::for_each(arg_symbols.begin(), arg_symbols.end(), [this](auto& sym)
	{
		sym.address = this->get_last_addr();
	});
}


void SymTable::update_proc_or_fun(int id, entry entry_type, std::vector<int>& args, int type)
{
	auto& symbol = this->check_symbol(id);

	if(entry_type != entry::FUNC and entry_type != entry::PROC)
	{
		throw CompilerException(interpolate("Unknown error. Expected FUNC or PROC, got {0}", entry_type), lineno);
	}

	symbol.m_entry = entry_type;
	symbol.m_scope = scope::GLOBAL;
	symbol.address = static_cast<int>(this->get_local_scope());

	if(type != SymTable::NONE)
	{
		
		auto offset = static_cast<int>(dtype::OBJECT);
		if (type >= offset)
		{
			auto type_id = type - offset;
			auto type_sym = this->get(type_id);

			if (type_sym.m_entry != entry::TYPE)
			{
				throw CompilerException(interpolate("Unknown error, expected TYPE entry not: {0}", type_sym.m_entry), lineno);
			}

			auto& function_result = this->get(this->insert(scope::LOCAL, 
														   symbol.name, 
														   static_cast<entry>(type_sym.address), 
														   type_sym.m_dtype, 
														   static_cast<int>(this->get_local_scope()),
														   true,
														   type_sym.start_ind,
														   type_sym.stop_ind));
			function_result.args = {type_sym};
			symbol.m_dtype = dtype::OBJECT;
		}
		else 
		{
			symbol.m_dtype = static_cast<dtype>(type);
			this->insert(scope::LOCAL, symbol.name, entry::VAR, static_cast<dtype>(type), static_cast<int>(this->get_local_scope()), true);
		}
	}

	this->update_addresses(args);
}

int SymTable::lookup(const std::string& name)
{
	const auto it =std::find_if(this->symbols.cbegin(), this->symbols.cend(), [&name, this](const Symbol& s)
	{
		return s.name == name and (
			s.m_entry == entry::FUNC or 
			s.m_entry == entry::RNG or 
			s.m_entry == entry::PROC or 
			s.m_entry == entry::OP or 
			s.m_entry == entry::TYPE or
			s.m_scope == this->get_scope()
		);
	});

	return it == this->symbols.cend() ? SymTable::NONE : it - this->symbols.cbegin();
}

Symbol& SymTable::get(const int id)
{
	return this->symbols[id];
}

void SymTable::update(Symbol& sym)
{
	if(sym.symtab_id == SymTable::NONE)
	{
		return;
	}

	this->symbols[sym.symtab_id].address = sym.address;
	this->symbols[sym.symtab_id].args = sym.args;
	this->symbols[sym.symtab_id].m_dtype = sym.m_dtype;
	this->symbols[sym.symtab_id].start_ind = sym.start_ind;
	this->symbols[sym.symtab_id].stop_ind = sym.stop_ind;
	this->symbols[sym.symtab_id].m_entry = sym.m_entry;
	this->symbols[sym.symtab_id].m_scope = sym.m_scope;
	this->symbols[sym.symtab_id].is_reference = sym.is_reference;
	this->symbols[sym.symtab_id].name = sym.name;
}

dtype SymTable::infer_type(Symbol& first, Symbol& second)
{
	if (first.m_dtype == dtype::NONE or second.m_dtype == dtype::NONE) return dtype::NONE;

	return first.m_dtype == second.m_dtype ? first.m_dtype : dtype::REAL;
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
		return s.m_entry == entry::VAR or s.m_entry == entry::ARR;
	});

	int addr = 0;

	if (it_to_last_sized != this->symbols.crend())
	{
		if (it_to_last_sized->m_scope == scope::GLOBAL)
		{
			addr = it_to_last_sized->address + it_to_last_sized->size();
		}
		else if (it_to_last_sized->m_scope == scope::LOCAL and it_to_last_sized->address < 0)
		{
			addr = it_to_last_sized->address - it_to_last_sized->size();
		}
	}

	return addr;
}

std::ostream& operator<<(std::ostream& out, const SymTable& symtab)
{
	out << std::endl;
	
	const auto& print_line = [&out](char fill, int width){
		out << std::setfill(fill) << std::setw(width) << "" << std::endl << std::setfill(' ');
	};

	print_line('=', 135);
	out << std::setw(10) << "scope" << "|" 
		<< std::setw(30) << "name" << "|" 
		<< std::setw(10) << "entry" << "|" 
		<< std::setw(15) << "is reference" << "|" 
		<< std::setw(50) << "type" << "|" 
		<< std::setw(15) << "address" << std::endl;
	print_line('=', 135);

	const auto& print_symbol = [&out, &print_line](const Symbol& symbol)
	{
		out << symbol;
		print_line('-', 135);
	};

	std::for_each(symtab.symbols.cbegin(), symtab.symbols.cend(), print_symbol);

	return out;
}