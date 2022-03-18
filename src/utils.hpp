#include <algorithm>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <iostream>

template<typename N>
std::string stringify(N const& val)
{
	std::stringstream ss;
	ss << (val);
	return ss.str();
}

template <typename S>
std::string interpolate(const S &orig)
{
	std::string out(orig);
	return out;
}

template <typename S, typename... Args>
std::string interpolate(const S &orig, const Args &...args) 
{
	std::string out(orig);

	auto tup = std::make_tuple(args...);
	
	size_t i = 0;
	
	std::apply([&i, &out](auto&& s...){
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
	}, tup);


  return out;
}