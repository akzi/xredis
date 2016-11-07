#include "detail/detail.hpp"
namespace xredis
{
	

	class redis_client
	{
	public:

		redis_client()
		{

		}
	private:
		std::list<std::string> send_buffer_queue_;
		reply_parser reply_parser_;
		xnet::connection conn_;
	};
	class xredis_cluster
	{
	public:
		xredis_cluster()
		{

		}
	private:

	};
	class xredis
	{
	public:
		xredis()
		{

		}
		bool hgetall(const std::string &key, string_map_callback callback)
		{

		}
	};
}