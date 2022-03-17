#include "symbol.hpp"
#include "compilerexception.hpp"
#include <string>
#include <vector>
#include <map>

extern int lineno;

class SymTable
{
	private:
		std::vector<Symbol> symbols;
		std::map<std::string, int> labels;
		int checkpoint = 0;
		int locals = 0;
		scope current_scope = scope::GLOBAL;
		local_scope current_local_scope = local_scope::UNBOUND;
		const static std::map<std::string, opcode> relops_mulops_signops;
		const static std::map<token, std::string> keywords;

	public:
		std::string keyword(const token);
		Symbol& get(const int);
		int get_last_addr();

		int insert(const scope&, const std::string&, const entry&,  const dtype&, int = SymTable::NONE, bool is_reference=false, int start=0, int stop=0); //general function
		int insert_temp(const dtype&, bool is_reference =false); //temporary
		int insert_constant(const std::string&, const dtype&); //constant
		int insert_label(const std::string&); //label
		int insert_by_token(const std::string&, const token&, const dtype= dtype::NONE); //identifier, constant or operator
		int insert_range(int, int); //range object
		int insert_array_type(std::vector<int>, dtype&); //begin array symbol
		void update_var(int, int, bool is_reference=false); //variable of id and type

		int lookup(const std::string&);

		void clear();

		scope scope();
		local_scope local_scope();
		void set_local_scope(enum local_scope&);
		void return_to_global_scope();
		void leave_global_scope();

		void create_checkpoint();
		void restore_checkpoint();

		dtype infer_type(Symbol&, Symbol&);

		constexpr static int NONE =-1;

		friend std::ostream& operator<<(std::ostream& out, const SymTable& symtab);
};