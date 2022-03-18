#include "enums.hpp"
#include "utils.hpp"
#include <vector> 
#include <iomanip>
#include <ostream>

struct Symbol 
{
	scope m_scope;
	entry m_entry;
	dtype m_dtype;
	std::string name;
	bool is_reference;
	int symtab_id;
	int address = 0xff;
	int start_ind;
	int stop_ind;
	std::vector<Symbol> args;
	
	std::string type_to_str() const;
	int size() const;

	std::string addr_to_str(bool dereference=false, bool callable=false) const;

	friend std::ostream& operator << (std::ostream& out, const Symbol& symbol);
};