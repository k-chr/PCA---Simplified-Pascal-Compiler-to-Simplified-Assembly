#include "global.hpp"
#include <memory>
#include <utility>
#include <cstdlib>
#include <iostream>

int lineno = 1;
std::shared_ptr<Emitter> emitter_ptr;
std::shared_ptr<SymTable> symtab_ptr;


std::tuple<std::string, std::string> parse_args(int argc, char* argv[])
{
	if(argc < 2)
	{
		throw CompilerException(interpolate("Not enough arguments provided to compiler, expected at least 2, got {0}", argc), -1);
	}

	std::string first, second;

	if(argc == 2)
	{
		first = argv[1];
	}
	else
	{
		first = argv[1];
		second = argv[2];
	}

	return std::make_tuple(first, second);
}

int main(int argc, char* argv[])
{
	std::string in, out;
	try 
	{
		std::tie(in, out) = parse_args(argc, argv);
	} 
	catch (CompilerException& ce) 
	{
		std::cerr << ce.what() << std::endl;
		return -1;
	}

	try
	{
		auto compiler = out.empty() ? Compiler(in) : Compiler(in, out);

		emitter_ptr = compiler.share_emitter();
		symtab_ptr = compiler.share_table();
		yyin = compiler.get_istream();

		compiler.compile();
	}
	catch (CompilerException& ce) 
	{
		std::cerr << ce.what() << std::endl;
		return -2;
	}
}