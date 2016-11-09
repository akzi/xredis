#pragma once
namespace xredis
{
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
				int port_;
				std::string ip_;
				std::string id_;
			};
			int min_slots_;
			int max_slots_;
			std::vector<node> node_;
		};
		std::vector<cluster_info> cluster_;
	};
}