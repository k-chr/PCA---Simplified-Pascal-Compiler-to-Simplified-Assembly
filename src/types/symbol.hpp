#include <string>
#include <vector>
#include "enums.hpp"

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
	
	std::string type_to_str()
	{
		std::string res = "None";
		switch (this->dtype) 
		{
			case dtype::INT:
			{
				res = "integer";
				break;
			}
			case dtype::REAL: 
			{
				res = "real";
				break;
			}
            case dtype::NONE:break;
        }
		return res;
    }

	int size() const
	{
		int aux = 0;
		int elems = 1;
		auto size = varsize::NONE;
		if (this->entry == entry::VAR or this->entry == entry::ARR)
		{
			if(this->is_reference) size = varsize::REF;
			else
			{
				switch (this->dtype) 
				{
					case dtype::INT: size = varsize::INT; break;
					case dtype::REAL: size = varsize::REAL; break;
					default: size = varsize::NONE; break;
				};
				if (this->entry == entry::ARR)
				{
					aux = static_cast<int>(varsize::INT) * this->args.size() * 2;
					for (const auto& dim : this->args)
					{
						int len = std::abs(dim.stop_ind - dim.start_ind);
						elems *= len;
					}

				}
			}
		}

		return static_cast<int>(size) * elems + aux;
	}

	std::string addr_to_str(bool dereference=false)
	{
		if (this->entry == entry::NUM or this->entry == entry::LABEL)
		{
			return "#" + this->name;
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
};