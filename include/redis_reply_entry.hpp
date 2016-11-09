#pragma once
namespace xredis
{
	struct cluster_slots
	{
		cluster_slots()
		{

		}
		cluster_slots(cluster_slots && self)
		{
			move_reset(std::move(self));
		}
		cluster_slots &operator=(cluster_slots &self)
		{
			move_reset(std::move(self));
			return *this;
		}
		void move_reset(cluster_slots && self)
		{
			if(&self == this)
				return;
			cluster_ = std::move(self.cluster_);
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