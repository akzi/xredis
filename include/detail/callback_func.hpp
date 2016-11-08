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

		struct cluster_slots 
 		{
  			cluster_slots()
  			{
  
  			}
  			cluster_slots(cluster_slots &&)
  			{
  
  			}
  			struct cluster_info 
  			{
  				struct node 
  				{
  					std::string ip_;
  					int port_;
  					std::string id_;
  				};
  				int min_slots_;
  				int max_slots_;
  				std::vector<node> node_;
  			};
  			std::vector<cluster_info> cluster_;
		};
		typedef typename callback_func<cluster_slots >::callback_handle  cluster_slots_callback;

		typedef typename callback_func<std::map<std::string, std::string>>::callback_handle string_map_callback;

		typedef typename callback_func<std::map<std::string, double>>::callback_handle double_map_callback;

		typedef typename callback_func<std::string>::callback_handle string_callback;

		typedef typename callback_func<uint32_t>::callback_handle integral_callback;
	}
}