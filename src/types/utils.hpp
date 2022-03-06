#include <string>
#include <vector>
#include <iostream>

template <typename S, typename... Args>
std::string interpolate(const S &orig, const Args &...args) 
{
	std::string out(orig);

	auto va = {args...};
	std::vector<std::string> v{va};

	size_t i = 0;

	for (auto &s : v) 
	{
		std::string is = std::to_string(i);
		std::string t = "{" + is + "}";
		try 
		{
			auto pos = out.find(t);
			if (pos != out.npos)
			{
				out.erase(pos, t.length()); 
				out.insert(pos, s);         
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