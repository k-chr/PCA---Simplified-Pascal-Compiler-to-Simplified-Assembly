#include <algorithm>
#include <exception>
#include <string>

class CompilerException: public std::exception
{
	private:
		std::string message;
	public:
		const int lineno;
		CompilerException(const std::string& what, const int lineno): std::exception(what.c_str()), lineno(lineno){
			auto what_c_str = std::exception::what();
			auto s = ((std::string(what_c_str) + (lineno >= 0 ? "\tEncountered at line: " + std::to_string(this->lineno) : "")));
			message =s;
		};
		
		const char* what() const noexcept override
		{
			return this->message.c_str();
		}
};