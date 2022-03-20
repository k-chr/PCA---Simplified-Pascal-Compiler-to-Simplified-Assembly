#include "compiler.hpp"

void Compiler::compile()
{
	this->parse_result = yyparse();

	this->output.close();
	std::fclose(this->input);
	this->input = nullptr;

	if(this->parse_result != 0)
	{
		std::remove(this->output_file_name.c_str());
	}
}

std::shared_ptr<SymTable> Compiler::share_table()
{
	return std::make_shared<SymTable>((this->symtable));
}

std::shared_ptr<Emitter> Compiler::share_emitter()
{
	return std::make_shared<Emitter>((this->emitter));
}

std::FILE* Compiler::get_istream() const
{
	return this->input;
}
