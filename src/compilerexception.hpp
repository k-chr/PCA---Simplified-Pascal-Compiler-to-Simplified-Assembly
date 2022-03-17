#include <exception>
#include <string>

class CompilerException: public std::exception
{
	public:
		const int lineno;
		CompilerException(const std::string& what, const int lineno): std::exception(what.c_str()), lineno(lineno){};
		
		std::string what()
		{
			auto what = std::exception::what();
			return std::string(what) + "\tEncountered at line: " + std::to_string(this->lineno);
		}
};