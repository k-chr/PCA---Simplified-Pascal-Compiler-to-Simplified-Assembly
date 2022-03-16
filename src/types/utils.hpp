#include <string>
#include <type_traits>
#include <vector>
#include <iostream>

inline std::string stringify(std::string const& val)
{
	return (val);
}

template<typename N, typename=std::enable_if_t<false == std::is_convertible<N, std::string>::value, std::string>>
std::string stringify(N const& val)
{
	return std::to_string(val);
}

template <typename S, typename... Args>
std::string interpolate(const S &orig, const Args &...args) 
{
	std::string out(orig);

	auto va = {args...};
	
	size_t i = 0;

	for (auto &s : va) 
	{
		std::string is = std::to_string(i);
		std::string t = "{" + is + "}";
		try 
		{
			auto pos = out.find(t);
			if (pos != out.npos)
			{
				out.erase(pos, t.length()); 
				out.insert(pos, stringify(s));         
			}

			i++;
		} 
		catch (std::exception &e) 
		{
			std::cerr << e.what() << std::endl;
		}
  }

  return out;
}