#include "emitter.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>

extern int yyparse();

class Compiler
{
	private:
		std::string file_name;
		std::string output_file_name;
		std::ofstream output;
		Emitter emitter;
		SymTable symtable;
		std::FILE* input;
		int parse_result = 0;
	
	public:
		std::shared_ptr<SymTable> share_table();
		std::shared_ptr<Emitter> share_emitter();
		void compile();
		Compiler(std::string file_name, std::string output_file_name="out.asm"):
																	   file_name(file_name),
																	   output_file_name(output_file_name), 
																	   output(output_file_name),
																	   emitter(output)
		{
			if(not this->output.is_open())
			{
				throw CompilerException(interpolate("Runtime error. Provided output file: \"{0}\" does not exist.", this->output_file_name), -1);
			}

			this->input = std::fopen(this->file_name.c_str(), "r");
			
			if(input == NULL)
			{
				this->output.close();
				std::remove(this->output_file_name.c_str());
				throw CompilerException(interpolate("Runtime error. Provided input file: \"{0}\" does not exist.", this->file_name), -1);
			}
		}

		std::FILE* get_istream() const;
};