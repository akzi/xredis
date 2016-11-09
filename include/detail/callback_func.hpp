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
			typedef std::function<void(std::string &&, T&&)> callback_handle;
		};

		typedef typename callback_func<cluster_slots >::callback_handle  cluster_slots_callback;

		typedef typename callback_func<std::map<std::string, std::string>>::callback_handle string_map_callback;

		typedef typename callback_func<std::map<std::string, double>>::callback_handle double_map_callback;

		typedef typename callback_func<std::string>::callback_handle bulk_callback;

		typedef typename callback_func<uint32_t>::callback_handle integral_callback;
	}
}