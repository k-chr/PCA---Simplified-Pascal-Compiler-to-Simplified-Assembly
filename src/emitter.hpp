#include "symtable.hpp"
#include <cmath>
#include <optional>
#include <stack>
#include <memory>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <type_traits>
#include <utility>
#include <algorithm>

extern int lineno;
extern std::shared_ptr<SymTable> symtab_ptr;

class Emitter
{		
	private:
		const static std::map<opcode, std::string> mnemonics;
		std::ostream &output;
		std::stringstream mem;
		std::stringstream temp_mem;
		std::string get_type_str(const dtype&);
		std::stack<std::vector<int>> params_stack;
		std::vector<int> params;

		void push(Symbol&);
		int reduce(Symbol&, std::vector<Symbol>&);
		void check_arrays(Symbol&, Symbol&);
		void check_bounds(Symbol&, Symbol&);
		int cast(Symbol&, dtype&);
		int negate(Symbol&);
		int boolean_negate(Symbol&);
		int binop(opcode, Symbol&, Symbol&, Symbol* result = nullptr);
		int relop(opcode, Symbol&, Symbol&, Symbol* result = nullptr);
		int andorop(opcode, Symbol&, Symbol&, Symbol* result = nullptr);
		int shift_pointer(Symbol&, Symbol&, Symbol* result = nullptr);
		void move_pointer(Symbol&, Symbol&);
		int begin_left_eval_or_and(opcode, int);
		int left_eval_and_or(int, int, bool or_op=false);
		void read(int);
		void write(int);
		void incsp(int);
		void assign(Symbol&, Symbol&);
		void label(Symbol&);
		void jump(Symbol&);
		void jump_if(Symbol&, Symbol&, Symbol&, opcode=opcode::EQ);
		void enter(int);

		void leave_subprogram();
		void commit_subprogram();

	public:
		Emitter();
		Emitter(std::ostream &output): output(output) {};
		Emitter(const Emitter& e): output(e.output) {};

		std::vector<int> get_params();
		void clear_params();
		void begin_parametric_expr();
		void end_parametric_expr();
		void store_param(int);
		void store_param_on_stack(int);

		void end_current_subprogram(int);

		int binary_op(int, int, int);
		int unary_op(int, int);
		int begin_or_else(int);
		int begin_and_then(int);
		int and_then(int, int);
		int or_else(int, int);
		int get_item(int);
		int variable_or_call(int, bool=false);
		void jump(int);
		void assign(int, int);
		std::optional<int> make_call(int, bool result_required=false);
		void call_program(int);
		void start_program(int);
		void end_program();
		void label(int);
		int cast(int, dtype&);
		int if_statement(int);
		int end_if();
		int begin_while();
		int while_statement(int);
		std::tuple<int, int> classic_for_statement(int, int, int, int);
		void classic_end_iteration(int, int, int);
		int repeat();
		void until(int, int);

		void write()
		{
			auto data = this->params;
			if(data.empty()) throw CompilerException("Syntax error. Write procedure expects at least one argument.", lineno);
			std::for_each(data.cbegin(), data.cend(), [this](auto symbol_id){this->write(symbol_id);});
		}

		template<typename... S, typename=std::enable_if_t<std::conjunction_v<std::is_same<int, S>...>>>
		void write(const S&... symbol_ids)
		{
			(write(symbol_ids), ...);
		}

		void read()
		{
			auto data = this->params;
			if(data.empty()) throw CompilerException("Syntax error. Read procedure expects at least one argument.", lineno);
			std::for_each(data.cbegin(), data.cend(), [this](auto symbol_id){this->read(symbol_id);});
		}

		template<typename... S, typename=std::enable_if_t<std::conjunction_v<std::is_same<int, S>...>>>
		void read(const S&... symbol_ids)
		{
			(read(symbol_ids), ...);
		}

		template<typename... Str, typename=std::enable_if_t<std::conjunction_v<std::is_same<std::string, Str>...>>>
		void emit_to_stream(const std::string& label, const std::string& op, const std::string& comment, const Str&... mnemonics)
		{
			auto& stream = this->get_stream();

  			stream << std::endl << std::setw(8) << std::left;
			int max_mnemonics = 3;
  			const auto& print = [&stream, &max_mnemonics](const std::string& item)
			{
				if (max_mnemonics-- < 3)
					stream << ",";
     			stream << std::setw(12) << item ;
  			};

  			stream << label<< std::setw(16);
  			stream << op << std::setw(0) << std::right;
  			(print(mnemonics), ...);

			for(auto i =0; i < max_mnemonics; ++i)
			{
				stream << std::setw(not i ? 12: 13) << "";
			}

			if(not comment.empty())
			{
				stream << std::setw(16) << "\t" << std::left;
				stream << comment << std::setw(0);
			}
			else
			{
				stream << std::setw(0) << std::left;
			}
		}

		std::ostream& get_stream();
};