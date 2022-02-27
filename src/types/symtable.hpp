#include "symbol.hpp"
#include <vector>
#include <map>

class SymTable
{
	private:
		std::vector<Symbol> symbols;
		std::map<std::string, int> labels;
		int checkpoint = 0;
		int locals = 0;
		scope current_scope = scope::GLOBAL;

	public:
		Symbol& get(const int);
		int get_last_addr();
		int insert(const std::string&, const entry&, const token&, const dtype&);
		int insert(const dtype&);
		int insert(const std::string&, const dtype&);
		int insert(const std::string&);
		int lookup(const std::string&);
		void clear();

		scope scope();
		void return_to_global_scope();
		void leave_global_scope();

		void create_checkpoint();
		void restore_checkpoint();

		dtype infer_type(Symbol&, Symbol&);

		const static int NONE =-1;
};