#include "symtable.hpp"
#include "compilerexception.hpp"
#include "utils.hpp"
#include <map>
#include <ostream>
#include <sstream>
#include <string>

extern int lineno;

class Emitter
{		
	private:
		const static std::map<opcode, std::string> mnemonics;
		const std::shared_ptr<SymTable> symtab_ptr;
		const std::ostream &output;
		const std::stringstream mem;
		std::string get_type_str(const dtype&);

	public:
		Emitter(const std::ostream &output, 
				const std::shared_ptr<SymTable> table): symtab_ptr(table), output(output) {};

		int binary_op(int, int, int);
		void jump(int);
		void jump_if(int, int, int, int);
		void make_call(int, const std::vector<int>&);
		void end_program();
		void label(int);
		void write(std::vector<int>&);
		void read(std::vector<int>&);
		int cast(int, dtype&);
		const std::ostream& get_stream();
};

