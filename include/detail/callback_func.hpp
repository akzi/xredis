#pragma once
#include <map>
#include <string>
namespace xredis
{
	namespace detail
	{
		template<typename T>
		struct callback_func
		{
			typedef std::function<void(std::string &, T&&)> callback_handle;
		};
		struct string_map_callback
		{
			typedef typename callback_func<std::map<std::string, std::string>>::callback_handle callback_handle;
		};
		struct double_map_callback 
		{
			typedef typename callback_func<std::map<std::string, double>>::callback_handle callback_handle;
		};
		struct string_callback 
		{
			typedef typename callback_func<std::string>::callback_handle callback_handle;
		};
		struct integral_callback
		{
			typedef typename callback_func<uint32_t>::callback_handle callback_handle;
		};
	}
}