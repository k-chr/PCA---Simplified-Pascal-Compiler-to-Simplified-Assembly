#include <string>
#include <vector>
#include "enums.hpp"

typedef struct Symbol Symbol;
struct Symbol 
{
	scope scope;
	entry entry;
	dtype dtype;
	token token;
	std::string name;
	int address;
	int start_idx;
	int stop_ind;
	std::vector<Symbol> args;
} ;