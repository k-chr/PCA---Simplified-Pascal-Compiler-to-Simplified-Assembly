#include <ostream>
#include <string>
#include <vector>
#include <iostream>    
#include <iomanip>
#include "enums.hpp"
#include "utils.hpp"

struct Symbol 
{
	scope scope;
	entry entry;
	dtype dtype;
	std::string name;
	bool is_reference;
	int symtab_id;
	int address = 0xff;
	int start_ind;
	int stop_ind;
	std::vector<Symbol> args;
	
	std::string type_to_str() const
	{
		std::string res = "NONE";

		if (this->entry == entry::ARR)
		{
			res = this->args[0].name;
		}
		else
		{
			res = stringify(this->dtype);	
		}

		return res;
    }

	int size() const
	{
		int elems = 1;
		auto size = varsize::NONE;
		if (this->is_reference)
		{
			size = varsize::REF;
		}
		else if (this->entry == entry::VAR or this->entry == entry::ARR) 
		{
			switch (this->dtype) 
			{
				case dtype::INT: size = varsize::INT; break;
				case dtype::REAL: size = varsize::REAL; break;
				default: size = varsize::NONE; break;
			};
			if (this->entry == entry::ARR)
			{
				int len = std::abs(this->stop_ind - this->start_ind);
				elems *= len;
			}
		}
		
		return static_cast<int>(size) * elems;
	}

	std::string addr_to_str(bool dereference=false, bool callable=false) const
	{
		if (this->entry == entry::NUM or this->entry == entry::LABEL or callable)
		{
			return "#" + this->name;
		}

		if (this->entry == entry::OP)
		{
			return stringify(static_cast<opcode> (this->address));
		}

		auto pos = std::to_string(this->address);
		std::string inter = this->scope == scope::LOCAL ? (this->address < 0 ? "BP" : "BP+") : "";
		std::string addr_op = "";
		if (this->is_reference and dereference)
		{
			addr_op = "*";	
		}
		else if (not this->is_reference and this->entry == entry::VAR and not dereference) 
		{
			addr_op = "#";
		}

		return addr_op + inter + pos;
	}

	friend std::ostream& operator << (std::ostream& out, const Symbol& symbol)
	{
		auto type = symbol.type_to_str();
		auto addr = symbol.addr_to_str(not symbol.is_reference);
		
		out << std::setw(10) << symbol.scope << "|" 
			<< std::setw(30) << (symbol.name.length() <= 30 ? symbol.name : symbol.name.substr(0, 27) + "...") << "|" 
			<< std::setw(10) << symbol.entry << "|" 
			<< std::setw(15) << symbol.is_reference << "|" 
			<< std::setw(50) << (type.length() <= 50 ? type : type.substr(0, 47) + "...") << "|" 
			<< std::setw(15) << (addr.length() <= 15 ? addr : addr.substr(0, 12) + "...") << std::endl;

		return out;
	}
};