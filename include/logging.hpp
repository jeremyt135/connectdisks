#pragma once

#include <iostream>
#include <string>

namespace util
{
	template<typename Arg0>
	void print(std::ostream& out, std::string sep, Arg0&& arg0)
	{
		out << arg0;
	}

	template<typename Arg0, typename... Args>
	void print(std::ostream& out, std::string sep, Arg0&& arg0, Args&&... args)
	{
		out << arg0;
		int dummy[sizeof...(Args)] = {(out << sep << args, 0)...};
	}
}


#if defined DEBUG || defined _DEBUG
#define printDebug(...) util::print(std::cout, " ", "[DEBUG]", __VA_ARGS__);
#else
#define printDebug(...) ((void)__VA_ARGS__)
#endif