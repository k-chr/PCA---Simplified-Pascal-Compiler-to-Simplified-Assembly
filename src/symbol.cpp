#include "symbol.hpp"
#include <cstdlib>

std::string Symbol::type_to_str() const
{
	std::string res = "NONE";
	if (this->m_entry == entry::ARR)
	{
		res = this->args[0].name;
	}
	else
	{
		res = stringify(this->m_dtype);	
	}
	return res;
}

int Symbol::size() const
{
	int elems = 1;
	auto size = varsize::NONE;
	
	if (this->is_reference)
	{
		size = varsize::REF;
	}

	else if (this->m_entry == entry::VAR or this->m_entry == entry::ARR) 
	{
		switch (this->m_dtype) 
		{
			case dtype::INT: size = varsize::INT; break;
			case dtype::REAL: size = varsize::REAL; break;
			default: size = varsize::NONE; break;
		};
		
		if (this->m_entry == entry::ARR)
		{
			int len = std::abs(this->stop_ind - this->start_ind);
			elems *= len;
		}
	}
	
	return static_cast<int>(size) * elems;
}

std::string Symbol::addr_to_str(bool dereference, bool callable) const
{
	if (this->m_entry == entry::NUM or this->m_entry == entry::LABEL or callable)
	{
		return "#" + this->name;
	}

	if (this->m_entry == entry::OP)
	{
		return stringify(static_cast<opcode> (this->address));
	}

	auto pos = std::to_string(this->address);
	std::string inter = this->m_scope == scope::LOCAL ? (this->address < 0 ? "BP" : "BP+") : "";
	std::string addr_op = "";

	if (this->is_reference and dereference)
	{
		addr_op = "*";	
	}

	else if (not this->is_reference and this->m_entry == entry::VAR and not dereference) 
	{
		addr_op = "#";
	}

	return addr_op + inter + pos;
}

std::ostream& operator << (std::ostream& out, const Symbol& symbol)
{
	auto type = symbol.type_to_str();
	auto addr = symbol.addr_to_str(not symbol.is_reference);
	
	out << std::setw(10) << symbol.m_scope << "|" 
		<< std::setw(30) << (symbol.name.length() <= 30 ? symbol.name : symbol.name.substr(0, 27) + "...") << "|" 
		<< std::setw(10) << symbol.m_entry << "|" 
		<< std::setw(15) << symbol.is_reference << "|" 
		<< std::setw(50) << (type.length() <= 50 ? type : type.substr(0, 47) + "...") << "|" 
		<< std::setw(15) << (addr.length() <= 15 ? addr : addr.substr(0, 12) + "...") << std::endl;
	return out;
}