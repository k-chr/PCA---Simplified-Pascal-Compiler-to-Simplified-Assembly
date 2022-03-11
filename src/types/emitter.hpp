#include "symtable.hpp"
#include "compilerexception.hpp"
#include "utils.hpp"
#include <cmath>
#include <ios>
#include <map>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>

extern int lineno;

class Emitter
{		
	private:
		const static std::map<opcode, std::string> mnemonics;
		const std::shared_ptr<SymTable> symtab_ptr;
		std::ostream &output;
		std::stringstream mem;
		std::string get_type_str(const dtype&);
		int cast(Symbol&, dtype&);
		int mulop(Symbol&, Symbol&, Symbol&);
		int relop(Symbol&, Symbol&, Symbol&);
		int andorop(Symbol&, Symbol&, Symbol&);

	public:
		Emitter(std::ostream &output, 
				const std::shared_ptr<SymTable> table): symtab_ptr(table), output(output) {};

		int binary_op(int, int, int);
		int unary_op(int, int);
		int and_then(int, int);
		int or_else(int, int);
		int get_item(int, int);
		void jump(int);
		void assign(int, int);
		void make_call(int, const std::vector<int>&);
		void end_program();
		void label(int);

		template<typename... S, typename=std::enable_if_t<std::conjunction_v<std::is_same<int, S>...>>>
		void write(int symbol_id, const S&... symbol_ids)
		{
			auto& symbol = this->symtab_ptr->get(symbol_id);
			auto& mnemonic = this->mnemonics.at(opcode::WRT);
			
			switch (symbol.entry)
			{		
                case entry::VAR:
                case entry::NUM:
				{
					std::string op = mnemonic + this->get_type_str(symbol.dtype);
					this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", op, symbol.name), symbol.addr_to_str(true));
					break;
				}
				
                case entry::ARR:
					throw CompilerException("No matching overload of write procedure for Array type", lineno);
				case entry::RNG:
					throw CompilerException(interpolate("Syntax error, expected constant or variable identifier, got a range object: {0}..{1}", symbol.start_ind, symbol.stop_ind), lineno);
                case entry::OP:
					throw CompilerException(interpolate("Syntax error, expected constant or variable identifier, got an operator: {0}", symbol.name), lineno);
                case entry::NONE:
                case entry::FUNC:
                case entry::PROC:
                case entry::LABEL:
                	throw CompilerException("Unknown error", lineno);
            }

			(write(symbol_ids), ...);
		}

		template<typename... S, typename=std::enable_if_t<std::conjunction_v<std::is_same<int, S>...>>>
		void read(int symbol_id, const S&... symbol_ids)
		{
			auto& symbol = this->symtab_ptr->get(symbol_id);
			auto& mnemonic = this->mnemonics.at(opcode::RD);

			switch (symbol.entry)
			{		
                case entry::VAR:
				{
					std::string op = mnemonic + this->get_type_str(symbol.dtype);
					this->emit_to_stream("\t\t", op, interpolate(";\t{0}\t{1}", op, symbol.name), symbol.addr_to_str(true));
					break;
				}
                case entry::NUM:
					throw CompilerException(interpolate("Syntax error, expected variable identifier, got an constant: {0}, of {1}", symbol.name, symbol.type_to_str()), lineno);
                case entry::ARR:
					throw CompilerException("No matching overload of read procedure for Array type", lineno);
				case entry::RNG:
					throw CompilerException(interpolate("Syntax error, expected variable identifier, got a range object: {0}..{1}", symbol.start_ind, symbol.stop_ind), lineno);
                case entry::OP:
					throw CompilerException(interpolate("Syntax error, expected variable identifier, got an operator: {0}", symbol.name), lineno);
                case entry::NONE:
                case entry::FUNC:
                case entry::PROC:
                case entry::LABEL:
                	throw CompilerException("Unknown error", lineno);
            }

			(read(symbol_ids), ...);
		}

		int cast(int, dtype&);

		template<typename... Str, typename=std::enable_if_t<std::conjunction_v<std::is_same<std::string, Str>...>>>
		void emit_to_stream(const std::string& label, const std::string& op, const std::string& comment, const Str&... mnemonics)
		{
			auto& stream = this->get_stream();

  			stream << std::endl << std::setw(8) << std::left;

  			const auto& print = [&stream](const std::string& item)
			{
     			stream << std::setw(12) << item << ", ";
  			};

  			stream << label<< std::setw(16);
  			stream << op << std::setw(0) << std::right;
  			(print(mnemonics), ...);

			if(stream.rdbuf() == std::cout.rdbuf())
			{
				stream << '\b' << '\b';
			}
			else 
			{
				stream.seekp(-2, std::ios_base::end);
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