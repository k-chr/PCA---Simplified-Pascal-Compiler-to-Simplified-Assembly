#include <exception>
#include <string>

class CompilerException: public std::exception
{
	private:
		std::string message;
	public:
		const int lineno;
		CompilerException(const std::string& what, const int lineno):  lineno(lineno){
			auto s = (what + (lineno >= 0 ? "\tEncountered at line: " + std::to_string(this->lineno) : ""));
			message = s;
		};
		
		const char* what() const noexcept override
		{
			return this->message.c_str();
		}
};